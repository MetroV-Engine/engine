# Logger.cpp holds the logger, LoggerDisabled.cpp the empty version. Debug
# builds get the first, every other build the second.
add_library(engine_logger STATIC
	$<IF:$<CONFIG:Debug>,Logger.cpp,LoggerDisabled.cpp>
)

target_include_directories(engine_logger
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/../..
)

# Logging exists in Debug builds only. Everything that links engine_logger
# gets the same value, so a whole program agrees on it.
target_compile_definitions(engine_logger
	PUBLIC
	$<$<CONFIG:Debug>:ENGINE_LOG_ENABLED>
)
