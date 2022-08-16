set(dir "${CMAKE_CURRENT_SOURCE_DIR}")

list(
  APPEND src
  "${dir}/LSM9DS1.cpp"
)

set(src "${src}" PARENT_SCOPE)