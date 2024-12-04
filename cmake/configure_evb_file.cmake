

file(WRITE ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "
<?xml version=\"1.0\" encoding=\"Windows-1252\"?>
<>
  <InputFile>${EVB_INSTALL_PREFIX}/PhyRecorder.exe</InputFile>
  <OutputFile>${EVB_INSTALL_PREFIX}/../PhyRecorder_release.exe</OutputFile>
  <Files>
    <Enabled>True</Enabled>
    <DeleteExtractedOnExit>False</DeleteExtractedOnExit>
    <CompressFiles>True</CompressFiles>
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
            file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "  <File>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <Type>3</Type>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <Name>${filename}</Name>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <Action>0</Action>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <OverwriteDateTime>False</OverwriteDateTime>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <OverwriteAttributes>False</OverwriteAttributes>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <HideFromDialogs>0</HideFromDialogs>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <Files>\n")
            scan_directory(${file})  # Recursively scan this subdirectory
            file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    </Files>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "  </File>\n")
        else()
            # It's a file, print its specific structure
            if(NOT extension STREQUAL ".txt")
                file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "  <File>\n")
                file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <Type>2</Type>\n")
                file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <Name>${filename}</Name>\n")
                file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <File>${file}</File>\n")
                file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <ActiveX>False</ActiveX>\n")
                file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <ActiveXInstall>False</ActiveXInstall>\n")
                file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <Action>0</Action>\n")
                file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <OverwriteDateTime>False</OverwriteDateTime>\n")
                file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <OverwriteAttributes>False</OverwriteAttributes>\n")
                file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <PassCommandLine>False</PassCommandLine>\n")
                file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <HideFromDialogs>0</HideFromDialogs>\n")
                file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "  </File>\n")
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
    if(IS_DIRECTORY ${file} AND NOT ${filename} STREQUAL "rec")
        file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "  <File>\n")
        file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <Type>3</Type>\n")
        file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <Name>${filename}</Name>\n")
        file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <Action>0</Action>\n")
        file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <OverwriteDateTime>False</OverwriteDateTime>\n")
        file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <OverwriteAttributes>False</OverwriteAttributes>\n")
        file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <HideFromDialogs>0</HideFromDialogs>\n")
        file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <Files>\n")
        scan_directory(${file})  # Recursively scan this subdirectory
        file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    </Files>\n")
        file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "  </File>\n")
    else()
        # It's a file, print its specific structure
        if(extension STREQUAL ".dll" OR extension STREQUAL ".ico")
            file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "  <File>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <Type>2</Type>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <Name>${filename}</Name>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <File>${file}</File>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <ActiveX>False</ActiveX>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <ActiveXInstall>False</ActiveXInstall>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <Action>0</Action>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <OverwriteDateTime>False</OverwriteDateTime>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <OverwriteAttributes>False</OverwriteAttributes>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <PassCommandLine>False</PassCommandLine>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "    <HideFromDialogs>0</HideFromDialogs>\n")
            file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "  </File>\n")
        endif()
    endif()
endforeach()


file(APPEND  ${EVB_INSTALL_PREFIX}/PhyRecorder.evb "
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