# ESP-IDF includes this file for every component in the build. It adds a target that
# runs once the firmware has been linked and copies the ELF to <project>/elf-archive/
# (archive_elf.cmake does the work). A panic can only be decoded with the ELF of the
# exact build that was running, and the next build overwrites build/firmware.elf.
#
# IDF only sets its EXECUTABLE build property after every project_include.cmake has been
# processed, so name the target the way project.cmake does: <project>.elf. The target
# itself is created later; add_dependencies() and $<TARGET_FILE:> resolve at generate time.
if(NOT CMAKE_PROJECT_NAME)
    message(WARNING "elf_archive: project name unknown, ELFs will not be archived")
    return()
endif()
set(elf_archive_target "${CMAKE_PROJECT_NAME}.elf")

# <project>/.esphome/build/<device> -> <project>
get_filename_component(elf_archive_project_dir "${CMAKE_SOURCE_DIR}/../../.." ABSOLUTE)

add_custom_target(archive_elf ALL
    COMMAND ${CMAKE_COMMAND}
            -DELF=$<TARGET_FILE:${elf_archive_target}>
            -DDEST_DIR=${elf_archive_project_dir}/elf-archive
            -DKEEP=20
            -P ${CMAKE_CURRENT_LIST_DIR}/archive_elf.cmake
    COMMENT "Archiving firmware ELF"
    VERBATIM
)
add_dependencies(archive_elf ${elf_archive_target})
