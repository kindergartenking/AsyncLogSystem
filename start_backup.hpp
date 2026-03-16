#include <boost/asio.hpp>
#include "TCPMgr.hpp"
#include <cstdint>
#pragma pack(push, 1)
struct Header {
    uint16_t id;
    uint16_t len;
};
#pragma pack(pop)

void start_backup(const std::string& data) {
    Header header;
    header.id = htons(1050);
    header.len = htons(static_cast<uint16_t>(data.size()));
    
    std::cout << "Host order ID: " << ntohs(header.id) << std::endl;
    std::cout << "Network order ID: " << header.id << std::endl;
    std::cout << "Host order len: " << ntohs(header.len) << std::endl;
    std::cout << "Network order len: " << header.len << std::endl;
    std::cout << "Header size: " << sizeof(header) << std::endl;
    // 2. 构建 Buffer 序列 (Scatter-Gather)
    // 这是一个“引用”的集合，不复制数据本身
    std::vector<boost::asio::const_buffer> buffers;

    // 第一段：指向栈上的 header
    buffers.push_back(boost::asio::buffer(&header, sizeof(header)));

    // 第二段：指向原来的 string data (零拷贝！)
    if (!data.empty()) {
        buffers.push_back(boost::asio::buffer(data));
    }

    // 3. 发送
    TCPMgr::GetInstance().sendBuffers(buffers);
}