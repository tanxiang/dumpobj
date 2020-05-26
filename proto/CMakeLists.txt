include(FindProtobuf)
PROTOBUF_GENERATE_CPP(PROTO_SRC PROTO_HEADER model.proto)

add_library(proto ${PROTO_HEADER} ${PROTO_SRC})