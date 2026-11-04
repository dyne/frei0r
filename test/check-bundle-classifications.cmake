# Match classifications to the configured inventory, independently of how many
# plugins are present in a PR head or its merge with the target branch.
if(NOT DEFINED CLASSIFICATION_TARGETS OR NOT DEFINED CLASSIFICATIONS)
  message(FATAL_ERROR "classification coverage requires targets and classifications")
endif()

set(unclassified_targets "${CLASSIFICATION_TARGETS}")
list(LENGTH unclassified_targets target_count)
list(REMOVE_DUPLICATES unclassified_targets)
list(LENGTH unclassified_targets unique_target_count)
if(NOT target_count EQUAL unique_target_count)
  message(FATAL_ERROR "duplicate target in the classification inventory")
endif()

foreach(classification IN LISTS CLASSIFICATIONS)
  if(NOT classification MATCHES "^([^|]+)[|]([^|]+)[|](.+)$")
    message(FATAL_ERROR "malformed target classification '${classification}'")
  endif()
  set(classified_target "${CMAKE_MATCH_1}")
  list(FIND unclassified_targets "${classified_target}" target_index)
  if(target_index EQUAL -1)
    message(FATAL_ERROR "unexpected or duplicate classification for '${classified_target}'")
  endif()
  list(REMOVE_ITEM unclassified_targets "${classified_target}")
endforeach()

if(unclassified_targets)
  message(FATAL_ERROR "missing classifications for targets: ${unclassified_targets}")
endif()
