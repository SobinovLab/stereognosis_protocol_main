set clientOutput=.\..\ProtocolApp\kinovaArm\
set serverOutput=.\..\pythonKinovaServer\
set protoFile=.\armMessages.proto

set grpc_cpp_plugin=D:\vcpkg\installed\x64-windows\tools\grpc\grpc_cpp_plugin.exe

protoc --plugin=protoc-gen-grpc=%grpc_cpp_plugin% --grpc_out=%clientOutput% %protoFile%
protoc --cpp_out=%clientOutput% %protoFile%

python -m grpc_tools.protoc -I . --python_out=%serverOutput% --pyi_out=%serverOutput% --grpc_python_out=%serverOutput% %protoFile%