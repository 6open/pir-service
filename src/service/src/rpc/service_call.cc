
#include "service_call.h"
#include "spdlog/spdlog.h"
#include "channel_brpc_gateway.h"

namespace yacl::link {

void SpuServiceCall::OnServiceOncall(const gateway_pb::Content& request_content,gateway_pb::ResponseContent& reponse_content)
{
    ic_pb::PushRequest request;
    ic_pb::PushResponse response;
    try {
        auto& bytedata = request_content.objectbytedata();
        request.ParseFromArray(bytedata.c_str(),bytedata.size());
        const size_t sender_rank = request.sender_rank();
        const auto& trans_type = request.trans_type();
        // dispatch the message
        if (trans_type == ic_pb::TransType::MONO) {
            OnRpcCall(sender_rank, request.key(), request.value());
        } else if (trans_type == ic_pb::TransType::CHUNKED) {
            const auto& chunk = request.chunk_info();
            OnRpcCall(sender_rank, request.key(), request.value(),
            chunk.chunk_offset(), chunk.message_length());
        } else {
            response.mutable_header()->set_error_code(
            ic::ErrorCode::INVALID_REQUEST);
            response.mutable_header()->set_error_msg(
            fmt::format("unrecongnized trans type={}, from rank={}", trans_type,
                        sender_rank));
        }
        response.mutable_header()->set_error_code(ic::ErrorCode::OK);
        response.mutable_header()->set_error_msg("");
    } catch (const std::exception& e) {
        response.mutable_header()->set_error_code(
            ic::ErrorCode::UNEXPECTED_ERROR);
        response.mutable_header()->set_error_msg(fmt::format(
            "dispatch error, key={}, error={}", request.key(), e.what()));
    }
    reponse_content.set_objectbytedata(response.SerializeAsString());
}

void SpuServiceCall::OnRpcCall(size_t src_rank, const std::string& key,
                const std::string& value) {
    auto itr = listeners_.find(src_rank);
    if (itr == listeners_.end()) {
        YACL_THROW_LOGIC_ERROR("dispatch error, listener rank={} not found",
                                src_rank);
    }
    // TODO: maybe need std::string_view interface to avoid memcpy
    return itr->second->OnMessage(key, value);
}

void SpuServiceCall::OnRpcCall(size_t src_rank, const std::string& key,
                const std::string& value, size_t offset, size_t total_length) {
    auto itr = listeners_.find(src_rank);
    if (itr == listeners_.end()) {
        YACL_THROW_LOGIC_ERROR("dispatch error, listener rank={} not found",
                                src_rank);
    }
    auto comm_brpc = std::dynamic_pointer_cast<ChannelBrpcGateWay>(itr->second);
    // TODO: maybe need std::string_view interface to avoid memcpy
    comm_brpc->OnChunkedMessage(key, value, offset, total_length);
}

}  // namespace yacl::link
