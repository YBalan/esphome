# Copies the linked firmware ELF into DEST_DIR as firmware-<date>_<time>-<id>.elf, where
# <id> is the first 8 hex digits of the ELF's SHA-256: the same value the device reports
# as "elf=" in "BMS Last Panic". Keeps the newest KEEP files.
#
#   cmake -DELF=<file> -DDEST_DIR=<dir> -DKEEP=<n> -P archive_elf.cmake
#
# Never fails the build: archiving is a convenience.
if(NOT EXISTS "${ELF}")
    message(WARNING "elf_archive: ${ELF} not found, nothing archived")
    return()
endif()

file(SHA256 "${ELF}" elf_sha)
string(SUBSTRING "${elf_sha}" 0 8 build_id)

file(MAKE_DIRECTORY "${DEST_DIR}")
file(GLOB already "${DEST_DIR}/firmware-*-${build_id}.elf")
if(already)
    list(GET already 0 existing)
    message(STATUS "elf_archive: build ${build_id} already archived: ${existing}")
else()
    string(TIMESTAMP stamp "%Y-%m-%d_%H%M")
    set(dest "${DEST_DIR}/firmware-${stamp}-${build_id}.elf")
    file(COPY_FILE "${ELF}" "${dest}" RESULT copy_result)
    if(copy_result)
        message(WARNING "elf_archive: copy to ${dest} failed: ${copy_result}")
        return()
    endif()
    message(STATUS "elf_archive: build ${build_id} archived as ${dest}")
endif()

# File names start with the date, so a plain sort is oldest-first.
file(GLOB all_elfs "${DEST_DIR}/firmware-*.elf")
list(SORT all_elfs)
list(LENGTH all_elfs count)
if(count GREATER KEEP)
    math(EXPR drop "${count} - ${KEEP}")
    list(SUBLIST all_elfs 0 ${drop} old_elfs)
    file(REMOVE ${old_elfs})
endif()
