if exist C:/Repositories/vcpkg/ (
set protoc=C:/Repositories/vcpkg/installed/x64-windows/tools/protobuf/protoc.exe
set grpc_cpp_plugin=C:/Repositories/vcpkg/installed/x64-windows/tools/grpc/grpc_cpp_plugin.exe
) else (
if exist C:/Repositories/vcpkg-master/ (
set protoc=C:/Repositories/vcpkg-master/installed/x64-windows/tools/protobuf/protoc.exe
set grpc_cpp_plugin=C:/Repositories/vcpkg-master/installed/x64-windows/tools/grpc/grpc_cpp_plugin.exe
) else (
echo "VCPKG not found. Aborting."
exit
)
)

set clientOutput=.\..\ProtocolApp\kinovaArm\
set serverOutput=.\..\pythonKinovaServer\
set protoFile=.\armMessages.proto


%protoc% --plugin=protoc-gen-grpc=%grpc_cpp_plugin% --grpc_out=%clientOutput% %protoFile%
%protoc% --cpp_out=%clientOutput% %protoFile%

python -m grpc_tools.protoc -I . --python_out=%serverOutput% --pyi_out=%serverOutput% --grpc_python_out=%serverOutput% %protoFile%