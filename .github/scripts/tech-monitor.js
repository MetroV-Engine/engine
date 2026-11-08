#!/usr/bin/env node
// Technology monitoring: polls each member's RSS/Atom feeds and pings them on
// Discord when a new item appears. Built-in modules only.
//
// Usage:
//   DISCORD_WEBHOOK_URL=<url> node .github/scripts/tech-monitor.js
//   node .github/scripts/tech-monitor.js --dry-run
//
// Options:
//   --dry-run          Print what would be posted; do not call Discord or write state.
//   --config <path>    Config file (default: .github/tech-monitor-config.json)
//   --state <path>     State file (default: last-seen.json at the repo root)
//
// A feed seen for the first time is "seeded": its current items are recorded
// as seen without posting, so adding a feed never floods the channel.

'use strict';

const fs = require('fs');
const path = require('path');
const http = require('http');
const https = require('https');
const crypto = require('crypto');

const REPO_ROOT = path.resolve(__dirname, '..', '..');
const MAX_ITEMS_PER_MESSAGE = 5;
const MAX_SEEN_PER_FEED = 200;
const FETCH_TIMEOUT_MS = 20000;
const USER_AGENT = 'MetroV-Engine-tech-monitor/1.0 (+https://github.com/MetroV-Engine/engine)';

function parseArgs(argv) {
  const opts = {
    dryRun: false,
    config: path.join(REPO_ROOT, '.github', 'tech-monitor-config.json'),
    state: path.join(REPO_ROOT, 'last-seen.json'),
  };
  for (let i = 0; i < argv.length; i++) {
    const arg = argv[i];
    if (arg === '--dry-run') opts.dryRun = true;
    else if (arg === '--config') opts.config = path.resolve(argv[++i]);
    else if (arg === '--state') opts.state = path.resolve(argv[++i]);
    else throw new Error(`Unknown argument: ${arg}`);
  }
  if (process.env.DISCORD_WEBHOOK_URL === 'DRY_RUN') opts.dryRun = true;
  return opts;
}

// ---------------------------------------------------------------- HTTP

function request(url, { method = 'GET', headers = {}, body = null } = {}, redirects = 5) {
  return new Promise((resolve, reject) => {
    const lib = url.startsWith('https:') ? https : http;
    const req = lib.request(url, { method, headers: { 'User-Agent': USER_AGENT, ...headers } }, (res) => {
      const { statusCode } = res;
      if (statusCode >= 300 && statusCode < 400 && res.headers.location && method === 'GET') {
        res.resume();
        if (redirects <= 0) return reject(new Error(`Too many redirects for ${url}`));
        const next = new URL(res.headers.location, url).toString();
        return resolve(request(next, { method, headers, body }, redirects - 1));
      }
      const chunks = [];
      res.on('data', (c) => chunks.push(c));
      res.on('end', () => resolve({ status: statusCode, headers: res.headers, body: Buffer.concat(chunks).toString('utf8') }));
    });
    req.setTimeout(FETCH_TIMEOUT_MS, () => req.destroy(new Error(`Timeout after ${FETCH_TIMEOUT_MS}ms`)));
    req.on('error', reject);
    if (body) req.write(body);
    req.end();
  });
}

// ---------------------------------------------------------------- Feed parsing

function decodeEntities(str) {
  return str
    .replace(/<!\[CDATA\[([\s\S]*?)\]\]>/g, '$1')
    .replace(/&#x([0-9a-f]+);/gi, (_, h) => String.fromCodePoint(parseInt(h, 16)))
    .replace(/&#(\d+);/g, (_, d) => String.fromCodePoint(parseInt(d, 10)))
    .replace(/&quot;/g, '"')
    .replace(/&apos;/g, "'")
    .replace(/&lt;/g, '<')
    .replace(/&gt;/g, '>')
    .replace(/&amp;/g, '&');
}

function tagText(block, tag) {
  const re = new RegExp(`<${tag}(?:\\s[^>]*)?>([\\s\\S]*?)</${tag}>`, 'i');
  const m = block.match(re);
  return m ? decodeEntities(m[1]).trim() : '';
}

function atomLink(block) {
  // Prefer rel="alternate" (or no rel), fall back to the first <link href>.
  const links = [...block.matchAll(/<link\b([^>]*?)\/?>/gi)].map((m) => m[1]);
  const attr = (attrs, name) => {
    const m = attrs.match(new RegExp(`\\b${name}\\s*=\\s*("([^"]*)"|'([^']*)')`, 'i'));
    return m ? decodeEntities(m[2] ?? m[3]) : '';
  };
  const alt = links.find((a) => /href=/i.test(a) && (!/\brel=/i.test(a) || /rel=["']alternate["']/i.test(a)));
  const any = links.find((a) => /href=/i.test(a));
  return alt ? attr(alt, 'href') : any ? attr(any, 'href') : '';
}

function parseFeed(xml) {
  const items = [];
  const rssItems = [...xml.matchAll(/<item\b[^>]*>([\s\S]*?)<\/item>/gi)].map((m) => m[1]);
  if (rssItems.length) {
    for (const block of rssItems) {
      const link = tagText(block, 'link') || atomLink(block);
      const guid = tagText(block, 'guid');
      items.push({ id: guid || link, title: tagText(block, 'title'), link });
    }
  } else {
    const entries = [...xml.matchAll(/<entry\b[^>]*>([\s\S]*?)<\/entry>/gi)].map((m) => m[1]);
    for (const block of entries) {
      const link = atomLink(block);
      const id = tagText(block, 'id') || tagText(block, 'yt:videoId');
      items.push({ id: id || link, title: tagText(block, 'title'), link });
    }
  }
  return items
    .map((it) => ({
      ...it,
      title: (it.title || '(untitled)').replace(/\s+/g, ' '),
      id: it.id || crypto.createHash('sha1').update(`${it.title}|${it.link}`).digest('hex'),
    }))
    .filter((it) => it.id);
}

async function fetchFeed(url, attempt = 1) {
  const res = await request(url, { headers: { Accept: 'application/rss+xml, application/atom+xml, application/xml, text/xml;q=0.9, */*;q=0.8' } });
  // Reddit rate-limits aggressively; back off and retry a couple of times.
  if ((res.status === 429 || res.status >= 500) && attempt < 3) {
    const retryAfter = Number(res.headers['retry-after']);
    const waitMs = Number.isFinite(retryAfter) && retryAfter > 0 ? Math.min(retryAfter, 30) * 1000 : 5000 * attempt;
    await new Promise((r) => setTimeout(r, waitMs));
    return fetchFeed(url, attempt + 1);
  }
  if (res.status !== 200) throw new Error(`HTTP ${res.status}`);
  const items = parseFeed(res.body);
  if (!items.length && !/<(rss|feed|rdf:RDF)\b/i.test(res.body)) throw new Error('Response is not an RSS/Atom feed');
  return items;
}

// ---------------------------------------------------------------- Discord

function escapeMarkdown(text) {
  return text.replace(/([\\*_~`|>\[\]])/g, '\\$1');
}

function buildMessage(member, feed, items) {
  const shown = items.slice(0, MAX_ITEMS_PER_MESSAGE);
  const noun = { reddit: 'post', youtube: 'video', newsletter: 'article' }[feed.type] || 'item';
  const header = items.length === 1
    ? `<@${member.discordUserId}> there is a new ${noun} on **${escapeMarkdown(feed.label)}**!`
    : `<@${member.discordUserId}> there are ${items.length} new ${noun}s on **${escapeMarkdown(feed.label)}**!`;
  const lines = shown.map((it) => `- [${escapeMarkdown(it.title).slice(0, 200)}](<${it.link}>)`);
  if (items.length > shown.length) lines.push(`- ...and ${items.length - shown.length} more`);
  let content = [header, ...lines].join('\n');
  if (content.length > 2000) content = content.slice(0, 1997) + '...';
  return {
    content,
    allowed_mentions: { parse: [], users: [member.discordUserId] },
  };
}

async function postWebhook(webhookUrl, payload, attempt = 1) {
  const body = JSON.stringify(payload);
  const res = await request(webhookUrl, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json', 'Content-Length': Buffer.byteLength(body) },
    body,
  });
  if (res.status === 429 && attempt <= 3) {
    let wait = 2;
    try { wait = JSON.parse(res.body).retry_after || wait; } catch { /* keep default */ }
    await new Promise((r) => setTimeout(r, Math.ceil(wait * 1000) + 250));
    return postWebhook(webhookUrl, payload, attempt + 1);
  }
  // Never include the webhook URL in errors: it is a secret.
  if (res.status < 200 || res.status >= 300) throw new Error(`Discord webhook returned HTTP ${res.status}`);
}

// ---------------------------------------------------------------- Main

function loadJson(file, fallback) {
  if (!fs.existsSync(file)) return fallback;
  try {
    return JSON.parse(fs.readFileSync(file, 'utf8'));
  } catch (err) {
    console.warn(`::warning::Could not parse ${path.basename(file)} (${err.message}); starting fresh`);
    return fallback;
  }
}

function isPlaceholderId(id) {
  return !/^\d{17,20}$/.test(id || '') || /^0+$/.test(id);
}

async function main() {
  const opts = parseArgs(process.argv.slice(2));
  const webhookUrl = process.env.DISCORD_WEBHOOK_URL;
  if (!opts.dryRun && !webhookUrl) {
    console.error('::error::DISCORD_WEBHOOK_URL is not set (use --dry-run to test without Discord)');
    process.exit(1);
  }

  const config = loadJson(opts.config, null);
  if (!config || !Array.isArray(config.members)) {
    console.error(`::error::Invalid or missing config: ${opts.config}`);
    process.exit(1);
  }
  const state = loadJson(opts.state, { feeds: {} });
  state.feeds = state.feeds || {};

  console.log(`tech-monitor: ${config.members.length} member(s)${opts.dryRun ? ' [dry-run]' : ''}`);
  let failures = 0;
  let posted = 0;

  for (const member of config.members) {
    const placeholder = isPlaceholderId(member.discordUserId);
    if (placeholder) console.log(`- ${member.name}: placeholder Discord ID, nothing will be posted`);

    for (const feed of member.feeds || []) {
      const key = `${member.discordUserId}|${feed.url}`;
      const label = feed.label || feed.url;
      let items;
      try {
        items = await fetchFeed(feed.url);
      } catch (err) {
        failures++;
        console.warn(`::warning::[${member.name}] ${label}: fetch failed: ${err.message}`);
        continue;
      }

      const known = state.feeds[key];
      const ids = items.map((it) => it.id);
      if (!known) {
        console.log(`  [seed] ${member.name} / ${label}: ${items.length} item(s) recorded, none posted (first run for this feed)`);
        if (opts.dryRun && items[0]) console.log(`         latest: "${items[0].title}" ${items[0].link}`);
        state.feeds[key] = { seen: ids.slice(0, MAX_SEEN_PER_FEED), updatedAt: new Date().toISOString() };
        continue;
      }

      const seen = new Set(known.seen);
      const fresh = items.filter((it) => !seen.has(it.id));
      if (!fresh.length) {
        console.log(`  [none] ${member.name} / ${label}: no new items`);
      } else {
        const payload = buildMessage(member, { ...feed, label }, fresh);
        if (opts.dryRun || placeholder) {
          console.log(`  [${opts.dryRun ? 'dry-run' : 'skip'}] ${member.name} / ${label}: ${fresh.length} new item(s), would post:`);
          console.log(payload.content.split('\n').map((l) => `      ${l}`).join('\n'));
        } else {
          try {
            await postWebhook(webhookUrl, payload);
            posted++;
            console.log(`  [sent] ${member.name} / ${label}: ${fresh.length} new item(s)`);
          } catch (err) {
            failures++;
            console.warn(`::warning::[${member.name}] ${label}: ${err.message}`);
            continue; // keep state unchanged so the items are retried next run
          }
        }
      }
      // Newest ids first, then previously seen ones, capped.
      const merged = [...new Set([...ids, ...known.seen])].slice(0, MAX_SEEN_PER_FEED);
      state.feeds[key] = { seen: merged, updatedAt: new Date().toISOString() };
    }
  }

  if (opts.dryRun) {
    console.log(`dry-run complete: state not written (${failures} failure(s))`);
  } else {
    fs.writeFileSync(opts.state, JSON.stringify(state, null, 2) + '\n');
    console.log(`done: ${posted} message(s) posted, ${failures} failure(s), state saved to ${path.basename(opts.state)}`);
  }
}

main().catch((err) => {
  console.error(`::error::${err.message}`);
  process.exit(1);
});
