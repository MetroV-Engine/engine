add_library(engine_logger STATIC
	Logger.cpp
)

target_include_directories(engine_logger
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/../..
)
