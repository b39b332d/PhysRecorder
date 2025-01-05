file(WRITE ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "
<?xml version=\"1.0\" encoding=\"Windows-1252\"?>
<>
  <InputFile>${EVB_INSTALL_PREFIX}/${EXEC_NAME}.exe</InputFile>
  <OutputFile>${EVB_INSTALL_PREFIX}/../${EXEC_NAME}_release.exe</OutputFile>
  <Files>
    <Enabled>True</Enabled>
    <DeleteExtractedOnExit>False</DeleteExtractedOnExit>
    <CompressFiles>False</CompressFiles>
    <Files>
      <File>
        <Type>3</Type>
        <Name>%DEFAULT FOLDER%</Name>
        <Action>0</Action>
        <OverwriteDateTime>False</OverwriteDateTime>
        <OverwriteAttributes>False</OverwriteAttributes>
        <HideFromDialogs>0</HideFromDialogs>
        <Files>
")
# Function to recursively scan a directory and generate XML-like structure
function(scan_directory dir)
    # Get all files and subdirectories in the current directory
    file(GLOB files_and_dirs LIST_DIRECTORIES true ${dir}/*)

    # Process each file/directory
    foreach(file ${files_and_dirs})
        # Check if it is a directory or a file
         get_filename_component(filename ${file} NAME)
         get_filename_component(extension ${file} LAST_EXT)
        if(IS_DIRECTORY ${file})
    if(NOT ${filename} STREQUAL "rc")
            file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "  <File>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <Type>3</Type>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <Name>${filename}</Name>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <Action>0</Action>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <OverwriteDateTime>False</OverwriteDateTime>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <OverwriteAttributes>False</OverwriteAttributes>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <HideFromDialogs>0</HideFromDialogs>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <Files>\n")
            scan_directory(${file})  # Recursively scan this subdirectory
            file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    </Files>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "  </File>\n")
            endif()
        else()
            # It's a file, print its specific structure
            if(NOT extension STREQUAL ".txt" AND NOT extension STREQUAL ".gitignore")
                file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "  <File>\n")
                file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <Type>2</Type>\n")
                file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <Name>${filename}</Name>\n")
                file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <File>${file}</File>\n")
                file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <ActiveX>False</ActiveX>\n")
                file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <ActiveXInstall>False</ActiveXInstall>\n")
                file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <Action>0</Action>\n")
                file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <OverwriteDateTime>False</OverwriteDateTime>\n")
                file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <OverwriteAttributes>False</OverwriteAttributes>\n")
                file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <PassCommandLine>False</PassCommandLine>\n")
                file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <HideFromDialogs>0</HideFromDialogs>\n")
                file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "  </File>\n")
            endif()
        endif()
    endforeach()
endfunction()

# Get all files and subdirectories in the current directory
file(GLOB files_and_dirs LIST_DIRECTORIES true ${EVB_INSTALL_PREFIX}/*)
# Process each file/directory
foreach(file ${files_and_dirs})
    # Check if it is a directory or a file
    get_filename_component(filename ${file} NAME)
    get_filename_component(extension ${file} LAST_EXT)
    if(IS_DIRECTORY ${file})
    if(NOT ${filename} STREQUAL "rec")
        file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "  <File>\n")
        file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <Type>3</Type>\n")
        file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <Name>${filename}</Name>\n")
        file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <Action>0</Action>\n")
        file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <OverwriteDateTime>False</OverwriteDateTime>\n")
        file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <OverwriteAttributes>False</OverwriteAttributes>\n")
        file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <HideFromDialogs>0</HideFromDialogs>\n")
        file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <Files>\n")
        scan_directory(${file})  # Recursively scan this subdirectory
        file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    </Files>\n")
        file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "  </File>\n")
        endif()
    else()
        # It's a file, print its specific structure
        if(extension STREQUAL ".dll" OR extension STREQUAL ".ico")
            file(SIZE ${file} filesize)
            if(extension STREQUAL ".dll" AND filesize GREATER 1024  AND NOT filename MATCHES "^(msvc|vcruntime|Qt[2-9]).*")
                list(APPEND UFX_CMD_LIST COMMAND ${UPX_EXE} -qq --force --best "${file}")
            endif()
            file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "  <File>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <Type>2</Type>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <Name>${filename}</Name>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <File>${file}</File>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <ActiveX>False</ActiveX>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <ActiveXInstall>False</ActiveXInstall>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <Action>0</Action>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <OverwriteDateTime>False</OverwriteDateTime>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <OverwriteAttributes>False</OverwriteAttributes>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <PassCommandLine>False</PassCommandLine>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "    <HideFromDialogs>0</HideFromDialogs>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "  </File>\n")
        endif()
    endif()
endforeach()

execute_process(${UFX_CMD_LIST})

file(APPEND  ${EVB_INSTALL_PREFIX}/${EXEC_NAME}.evb "
        </Files>
      </File>
    </Files>
  </Files>
  <Registries>
    <Enabled>False</Enabled>
    <Registries>
      <Registry>
        <Type>1</Type>
        <Virtual>True</Virtual>
        <Name>Classes</Name>
        <ValueType>0</ValueType>
        <Value/>
        <Registries/>
      </Registry>
      <Registry>
        <Type>1</Type>
        <Virtual>True</Virtual>
        <Name>User</Name>
        <ValueType>0</ValueType>
        <Value/>
        <Registries/>
      </Registry>
      <Registry>
        <Type>1</Type>
        <Virtual>True</Virtual>
        <Name>Machine</Name>
        <ValueType>0</ValueType>
        <Value/>
        <Registries/>
      </Registry>
      <Registry>
        <Type>1</Type>
        <Virtual>True</Virtual>
        <Name>Users</Name>
        <ValueType>0</ValueType>
        <Value/>
        <Registries/>
      </Registry>
      <Registry>
        <Type>1</Type>
        <Virtual>True</Virtual>
        <Name>Config</Name>
        <ValueType>0</ValueType>
        <Value/>
        <Registries/>
      </Registry>
    </Registries>
  </Registries>
  <Packaging>
    <Enabled>False</Enabled>
  </Packaging>
  <Options>
    <ShareVirtualSystem>False</ShareVirtualSystem>
    <MapExecutableWithTemporaryFile>True</MapExecutableWithTemporaryFile>
    <TemporaryFileMask/>
    <AllowRunningOfVirtualExeFiles>True</AllowRunningOfVirtualExeFiles>
    <ProcessesOfAnyPlatforms>False</ProcessesOfAnyPlatforms>
  </Options>
  <Storage>
    <Files>
      <Enabled>False</Enabled>
      <Folder>%DEFAULT FOLDER%/</Folder>
      <RandomFileNames>False</RandomFileNames>
      <EncryptContent>False</EncryptContent>
    </Files>
  </Storage>
</>
")