#pragma once
//#include <wx/panel.h>
//#include <wx/textctrl.h>
//#include <wx/button.h>
//#include <wx/webview.h>
//#include <wx/bitmap.h>
#include <wx/wx.h>
//#include <wx/statbmp.h>
//#include <wx/notebook.h>
#include <wx/textfile.h>
#include <wx/url.h>
#include <wx/tokenzr.h>
#include <wx/stream.h>
#include <wx/uri.h>
//#include <wx/sstream.h>
#include <iostream>
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
//#include <boost/beast.hpp>
//#include <boost/asio.hpp>
#include <unordered_map>
#include "nlohmann/json.hpp" 
#include <cmath>
//#ifndef wxCUSTOMT_evt22
//wxDECLARE_EVENT(wxCUSTOMT_evt22, wxCommandEvent);
//wxDEFINE_EVENT(wxCUSTOMT_evt22, wxCommandEvent);
//#endif
//wxEVT_BUTTON
//const wxEventTypeTag<wxCommandEvent> wxCUSTOMT_evt22(wxNewEventType());
using json = nlohmann::json;
namespace pt    = boost::property_tree;
//namespace beast = boost::beast;         // from <boost/beast.hpp>
//namespace http  = beast::http;          // from <boost/beast/http.hpp>
//namespace net   = boost::asio;          // from <boost/asio.hpp>
//using tcp       = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>
//CircularImageButton* bt = dynamic_cast<CircularImageButton*>(event.GetEventObject());

#define MAX_PATH_Len 260

// 定义文件信息结构体
struct m_FileInfo
{
    wxString name;
    wxString lastPrinted;
    wxString modified;
    wxString size;
    long      imgidx;
};

 // 网络请求及JSON解析类
class HttpJsonClient
{
public:
    // 定义后缀到Content-Type的映射表（常见类型）
    static inline std::unordered_map<wxString, wxString> g_mimeTypes = {
        // 图片类型
        {"png", "image/png"},
        {"jpg", "image/jpeg"},
        {"jpeg", "image/jpeg"},
        {"gif", "image/gif"},
        {"bmp", "image/bmp"},
        {"webp", "image/webp"},
        {"svg", "image/svg+xml"},
        // 文本类型
        {"txt", "text/plain"},
        {"html", "text/html"},
        {"css", "text/css"},
        {"js", "text/javascript"},
        {"xml", "text/xml"},
        // 文档类型
        {"pdf", "application/pdf"},
        {"doc", "application/msword"},
        {"docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
        {"xls", "application/vnd.ms-excel"},
        {"xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
        // 压缩文件
        {"zip", "application/zip"},
        {"rar", "application/vnd.rar"},
        {"tar", "application/x-tar"},
        {"gz", "application/gzip"},
        // 其他常见类型
        {"json", "application/json"},
        {"mp3", "audio/mpeg"},
        {"mp4", "video/mp4"},
        {"exe", "application/x-msdownload"}};

    // 根据文件名或后缀获取Content-Type
    static wxString GetContentTypeByExtension(const wxString& filename)
    {
        wxString ext;
        if (filename.Find('.') != wxNOT_FOUND) {
            // 从文件名中提取后缀（忽略大小写）
            ext = wxFileName(filename).GetExt().Lower();
        } else {
            // 直接使用传入的后缀（忽略大小写）
            ext = filename.Lower();
        }
        // 查找映射表
        auto it = g_mimeTypes.find(ext);
        if (it != g_mimeTypes.end()) {
            return it->second;
        }
        // 未知类型返回通用二进制流类型
        return "application/octet-stream";
    }

     //发送GET请求并返回解析后的JSON对象
    static bool GetPrinterJson(const wxString& url, wxString* result)
    {
        pt::ptree root;
        if(!ReadTree(url, root,1)){
            return false;
        }
        try {
            pt::ptree   tmep     = root.get_child("result");                   
            std::string state    = tmep.get<std::string>("state");
            std::string hostname = tmep.get<std::string>("hostname");
            if (state == "ready") {
                *result = wxString(hostname);
                return true;
            }
        }
        catch (std::exception const& e) { 
             std::cerr << "Read Json error: " << e.what() << std::endl;
         }
     
      return false;
    }
     // 读取文本文件内容并返回
    static bool read_text_file(const fs::path& file_path, wxArrayString* host_Ip_list)
     {
        bool found = false;
         try {
             // 检查文件是否存在且是普通文件
             if (!fs::exists(file_path)) {
                 std::cout << file_path.string() << " not exists!" << std::endl;
                 //throw std::runtime_error("文件不存在: " + file_path.string());
                 return false;
             }
             if (!fs::is_regular_file(file_path)) {
                 throw std::runtime_error("not an ordinary file: " + file_path.string());
             }
             // 打开文件并读取内容
             std::ifstream file(file_path.string());
             if (!file.is_open()) {
                 throw std::runtime_error("could not open file:" + file_path.string());
             }

             // 读取全部内容到字符串
             std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
             pt::ptree   root;
             // std::istringstream iss(responseData);
             try {
                 std::istringstream iss(content);
                 pt::read_json(iss, root); 
                 // 检查是否包含printer数组
                 if (root.find("printer") == root.not_found()) {
                     throw std::runtime_error("printer not exists");
                 }

                // 遍历printer数组 
                 for (const auto& printer_node : root.get_child("printer"))
                 {
                     // printer_node.first是空字符串，printer_node.second是数组元素
                     const pt::ptree& printer = printer_node.second;
                     // 检查是否包含ip和state字段
                     if (printer.find("ip") == printer.not_found() || printer.find("state") == printer.not_found()) {
                         continue; // 跳过缺少字段的元素
                     }
                     try {
                         // 获取ip和state值
                         std::string ip    = printer.get<std::string>("ip");
                         bool        state = printer.get<bool>("state");
                         // 找到第一个state为true的IP
                         if (!found && state) {
                             found    = true;
                             host_Ip_list->Item(0) = wxString(ip);
                             //host_Ip_list->Insert(wxString(ip), 0);
                         } else
                             host_Ip_list->Add(wxString(ip));
                     } catch (const pt::ptree_error& e) {
                         // 处理类型转换错误（如state不是布尔值）
                         std::cerr << "Read printer info erro: " << e.what() << std::endl;
                         continue;
                     }
                 }
                 return found;
             } catch (std::exception const& e) {
                 std::cerr << "Read Json error: " << e.what() << std::endl;
                 return false;
             }
         } catch (const fs::filesystem_error& e) {
             //throw std::runtime_error("文件系统错误: " + std::string(e.what()));
             std::cerr << "Filesystem error: " << e.what() << std::endl;
             return false;
         }
     }
     static bool InputStreamToFile(wxInputStream& inputStream, const wxString& filePath)
     {
         // 检查输入流状态
         if (!inputStream.IsOk()) {
             return false;
         }
         // 创建文件输出流（覆盖模式）
         wxFileOutputStream fileOut(filePath);
         if (!fileOut.IsOk()) {
             return false;
         }
         // 定义缓冲区大小（可根据需求调整，一般4KB~64KB）
         const size_t BUFFER_SIZE = 8192; // 8KB缓冲区
         char         buffer[BUFFER_SIZE];

         // 循环读取并写入数据
         while (true) {
             // 从输入流读取数据到缓冲区
             inputStream.Read(buffer, BUFFER_SIZE);
             size_t bytesRead = inputStream.LastRead();
             // 检查是否读取到数据
             if (bytesRead == 0) {
                 // 读取结束或出错
                 break;
             }
             // 将缓冲区数据写入文件
             fileOut.Write(buffer, bytesRead);
             if (!fileOut.IsOk()) {
                 wxLogError("写入文件失败: %s", filePath);
                 wxRemoveFile(filePath); // 清理不完整文件
                 return false;
             }
         }
         // 检查最终状态（是否正常结束）
         if (!inputStream.Eof() || inputStream.GetLastError() != wxSTREAM_NO_ERROR) {
             wxLogError("读取输入流时发生错误");
             wxRemoveFile(filePath);
             return false;
         }
         return true;
     }
    static wxInputStream* InitHttpConnection(
         wxHTTP* http, const wxString& strurl, int timeout = 5, wxString sMethod = "GET", wxString header[] = nullptr)
    {
        // 解析URL
        wxURL urlParser(strurl);
        if (!urlParser.IsOk()) {
            return nullptr;
        }
        // 提取主机名和路径
        wxString server = urlParser.GetServer();
        wxString path   = urlParser.GetPath();
        long      port;
        urlParser.GetPort().ToLong(&port);
        if (path.IsEmpty())
            path = "/";
        // 添加查询参数
        wxString query = urlParser.GetQuery();
        if (!query.IsEmpty())
            path += "?" + query;
        http->SetTimeout(timeout); // 设置超时时间为1秒
        http->SetMethod(sMethod);

        /*if (header) {
            for (int i = 0; !header[i+1].IsEmpty(); i += 2) {
                http->SetHeader(header[i], header[i + 1]);
            }
        }*/
        // 连接服务器
        if (!http->Connect(server, port)) {
            return nullptr;
        }
        return http->GetInputStream(path); 
    }

    // 发送GET请求并获取响应数据
    static bool SendGetRequest(const wxString& url, wxString& response, int timeout = 5, wxString sMethod = "GET")
    {
        wxHTTP http;
        //std::cout << "SendGetRequest: " << url << std::endl;
        //wxString       header[4] = {"Accept", "application/json", "User-Agent", "wxWidgets Client"};
        http.SetHeader("Accept", "application/json");
        http.SetHeader("User-Agent", "wxWidgets Client");
        wxInputStream* stream   = InitHttpConnection(&http, url, timeout, sMethod);
        std::cout << "GetResponse: " << http.GetResponse() << std::endl;
        if (stream && http.GetResponse() == 200) // 200表示请求成功
        {
            // 读取响应数据
            wxStringOutputStream os(&response);
            os << *stream;
            delete stream;
            http.Close();
            return true;
        }
        delete stream;
        http.Close();
        return false;
    }

    static bool SendPostMemory(const wxString& url, wxMemoryBuffer* postBuffer, const wxString& contentType, wxString& response, int timeout =5)
    {
        wxHTTP   http;
        http.SetPostBuffer(contentType, *postBuffer); 
        http.SetHeader("Accept", "application/json");
        http.SetHeader("User-Agent", "wxWidgets Client");
        wxInputStream* stream = InitHttpConnection(&http, url, timeout, "POST");
        if (!stream || !stream->IsOk()) {
            //std::cout << "GetResponse: " << http.GetResponse() << std::endl;
            http.Close();
            return false;
        }
        wxStringOutputStream responseStream(&response);
        responseStream << *stream;
        delete stream;
        //std::cout << "GetResponse: " << http.GetResponse() << std::endl;
        http.Close();
        return true;
    }
    
    static bool SendPostRequest(
        const wxString& url, const wxString& postData, const wxString& contentType, wxString& response, int timeout = 5)
    {
        wxHTTP http;
        http.SetPostText(contentType, postData);         
        http.SetHeader("Accept", "application/json");
        http.SetHeader("User-Agent", "wxWidgets Client");
        wxInputStream* stream = InitHttpConnection(&http, url, timeout, "POST");
        if (!stream || !stream->IsOk()) {
            //std::cout << "GetResponse: " << http.GetResponse() << std::endl;
            http.Close();
            return false;
        }
        wxStringOutputStream responseStream(&response);
        responseStream << *stream;
        delete stream;
        //std::cout << "GetResponse: " << http.GetResponse() << std::endl;
        http.Close();
        return true;
    }
    
    static bool ReadTree(const wxString& url, pt::ptree& root, int timeout = 5)
    { 
        wxString response;
        //wxHTTP   http;
        if (!SendGetRequest(url, response, timeout)) {
            return false;
        }
        try {
            std::istringstream iss(response.ToStdString());
            pt::read_json(iss, root); // 核心解析函数
            return true;
        } catch (std::exception const& e) {
            std::cerr << "Read Json error: " << e.what() << std::endl;
        }
        return false;
    }
    
    static bool Write_cursorIp(const wxString& filePath, wxString& ip, bool state = true)
    {
        try {

            pt::ptree printertree1; // 新的printer数组
            pt::ptree newPrinter;
            newPrinter.put("ip", ip.ToStdString());
            newPrinter.put("state", state);
            printertree1.push_back(std::make_pair("", newPrinter));
            // 如果文件存在，读取并处理原有内容
            std::ifstream fileCheck(filePath.ToStdString());
            if (fileCheck.good()) // 检查文件是否存在且可读取
            {
                fileCheck.close();
                std::ifstream file(filePath.ToStdString());
                // 读取全部内容到字符串
                std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                pt::ptree   rootold;
                // std::istringstream iss(responseData);
                try {
                    std::istringstream iss(content);
                    pt::read_json(iss, rootold);
                    // 检查是否包含printer数组
                    if (rootold.find("printer") != rootold.not_found()) {
                        for (const auto& printer_node : rootold.get_child("printer")) {
                            std::string existingIp = printer_node.second.get<std::string>("ip");
                            // 检查是否包含ip和state字段
                            if (existingIp != ip) {
                                printertree1.push_back(std::make_pair("", printer_node.second));
                            }
                        }
                    }
                } catch (std::exception const& e) {
                    std::cerr << "Read Json error: " << e.what() << std::endl;
                    return false;
                }
            } else {
                std::cout << "注意：文件不存在，将创建新文件" << std::endl;
                fileCheck.close();
            }
            pt::ptree root;
            root.add_child("printer", printertree1);
            //std::ofstream outFile(filePath);
            //if (!outFile.is_open()) {
            //    std::cerr << "错误：无法打开文件进行写入 - " << filePath << std::endl;
            //    return false;
            //}
            // 写入JSON，使用缩进格式便于阅读
            pt::write_json(filePath.ToStdString(), root, std::locale(), false);
            //outFile.close();

            return true;
        } catch (const pt::json_parser_error& e) {
            std::cerr << "JSON erro：" << e.what() << "（line：" << e.line() << "）" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "JSON erro2：" << e.what() << std::endl;
        }
        return false;
    }

    static wxString GenerateBoundary()
    {
        wxString       boundary;
        //wxRandom       rand;
        //const wxString chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
        boundary = "----MoonrakerUploadahchei";
        //for (int i = 0; i < 16; ++i) {
        //    boundary += chars[rand.GetRange(0, chars.Length() - 1)];
        //}
        return boundary;
    }

    static bool PostGocde(const wxString& ipname, const wxString& gcode) { 
        if (ipname == "")
            return false;
        wxString url      = wxString::Format("http://%s/printer/gcode/script", ipname);
        wxString jsoncode = wxString::Format("{\"script\": \"%s\"}", gcode);
        wxString response, error;
        if (HttpJsonClient::SendPostRequest(url, jsoncode, "application/json", response)) {
            std::cout << "Response: " << response << std::endl;
            return true;
        } else {
            std::cerr << "Error: " << error << std::endl;
            return false;
        }
    }

    static bool UploadFile(wxString url, wxString localFilePath)
    {
        // 检查本地文件是否存在
        if (!wxFileExists(localFilePath)) {
            return false;
        }
        // 获取文件名和Content-Type
        wxFileName fileName(localFilePath);
        wxString contentType = HttpJsonClient::GetContentTypeByExtension(fileName.GetExt());
        // 打开本地文件（二进制模式）
        std::ifstream file(localFilePath.ToStdString(), std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            return false;
        }
        // 获取文件大小（通过移动到文件末尾获取位置）
        std::streamsize fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        wxString boundary = GenerateBoundary();
        wxString header = wxString::Format("--%s\r\nContent-Disposition: form-data; name=\"file\"; filename=\"%s\"\r\n"
                                           "Content-Type: %s\r\n\r\n", boundary, fileName.GetFullName() , contentType);
        wxString footer = wxString::Format("\r\n--%s--\r\n", boundary);
        size_t totalSize = header.Length() + fileSize + footer.Length();
        // 创建二进制缓冲区存储所有数据
        wxMemoryBuffer buffer(totalSize);
        char*          ptr = static_cast<char*>(buffer.GetWriteBuf(totalSize));
        memcpy(ptr, header.c_str(), header.Length());
        ptr += header.Length();
        // 读取并写入文件内容（二进制）
        if (!file.read(ptr, fileSize)) {
            //buffer.clear();
            file.close();
            return false;
        }
        ptr += fileSize;
        file.close();
        memcpy(ptr, footer.c_str(), footer.Length());
        buffer.UngetWriteBuf(totalSize);
        // 构建Content-Type
        wxString fullContentType = wxString::Format("multipart/form-data; boundary=%s", boundary);
        wxString response;
        wxHTTP   http;
        http.SetPostBuffer(contentType, buffer);
        http.SetHeader("Accept", "application/json");
        http.SetHeader("User-Agent", "wxWidgets Client");
        wxInputStream* stream = InitHttpConnection(&http, url, 5, "POST");
        if (!stream || !stream->IsOk()) {
            std::cout << "GetResponse: " << http.GetResponse() << std::endl;
            http.Close();
            return false;
        }
        wxStringOutputStream responseStream(&response);
        responseStream << *stream;
        delete stream;
        std::cout << "GetResponse: " << http.GetResponse() << std::endl;
        http.Close();
        return true;
    }

    static void print_tree(pt::ptree* tree, bool type = true) // type = true格式化输出，false紧凑输出
    {
        if (tree) {
            std::ostringstream oss;
            pt::write_json(oss, *tree, type);
            std::cout << oss.str() << endl;
        }
    }

   // 获取本机所有局域网 IPv4 地址（兼容 Boost < 1.66）
    static std::vector<std::string> get_local_ipv4_addresses()
    {
        std::vector<std::string> ips;
        boost::asio::io_context  io_context;
        try {
            // 1. 获取本机主机名
            std::string hostname = boost::asio::ip::host_name();
            // 2. 创建 resolver 解析主机名
            boost::asio::ip::tcp::resolver        resolver(io_context);
            boost::asio::ip::tcp::resolver::query query(hostname, ""); // 空端口表示仅解析 IP
            // 3. 遍历解析结果，筛选 IPv4 非回环地址
            for (auto it = resolver.resolve(query); it != boost::asio::ip::tcp::resolver::iterator(); ++it) {
                boost::asio::ip::address addr = it->endpoint().address();
                // 只保留 IPv4 且非回环地址
                if (addr.is_v4() && !addr.to_v4().is_loopback()) {
                    ips.push_back(addr.to_string());
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "获取 IP 失败: " << e.what() << std::endl;
        }
        return ips;
    }
};
struct jsonrpcInfo
{
    int   id;    
    char* method;
    char* params;
    int   resend = 0;
    char* text = "";
};
struct WebSocketFrameHeader
{
    bool     fin;         // 是否为消息的最后一帧
    bool     mask;        // 载荷是否被掩码处理
    uint8_t  opcode;      // 操作码 (1:文本, 2:二进制, 8:关闭, 9:Ping, 10:Pong)
    uint64_t p_len; // 载荷数据长度
    size_t   h_len; // 整个帧头的字节数 (用于从数据流中定位 payload 的起始位置)
};
class WebSocket_m
{
public:
    // 辅助函数：将16位无符号整数转换为大端字节序
    static std::vector<uint8_t> ToBigEndian(uint16_t value)
    {
        return {static_cast<uint8_t>((value >> 8) & 0xFF), static_cast<uint8_t>(value & 0xFF)};
    }

    // 辅助函数：将64位无符号整数转换为大端字节序
    static std::vector<uint8_t> ToBigEndian(uint64_t value)
    {
        std::vector<uint8_t> bytes(8);
        for (int i = 7; i >= 0; --i) {
            bytes[7 - i] = static_cast<uint8_t>((value >> (i * 8)) & 0xFF);
        }
        return bytes;
    }

    static bool DecodeWebSocketFrameHeader(const void* data1, size_t len, WebSocketFrameHeader& header)
    {
        // 至少需要 2 个字节才能开始解析
        if (len < 2) {
            return false;
        }
        uint8_t* data = (uint8_t*) data1;
        // 解析第一个字节
        header.fin    = (data[0] & 0x80) != 0;
        header.opcode = data[0] & 0x0F;
        header.mask                   = (data[1] & 0x80) != 0;
        uint8_t payload_len_indicator = data[1] & 0x7F;
         //  解析 payload 长度和帧头大小
        size_t header_size = 2; // 基础头部大小
        if (payload_len_indicator <= 125) {
            header.p_len = payload_len_indicator;
        } else if (payload_len_indicator == 126) {
            // 需要额外 2 个字节来表示长度
            if (len < header_size + 2) {
                return false;
            }
            header.p_len = (static_cast<uint64_t>(data[2]) << 8) | data[3];
            header_size += 2;
        } else { // payload_len_indicator == 127
            // 需要额外 8 个字节来表示长度
            if (len < header_size + 8) {
                return false;
            }
            header.p_len = 0;
            for (int i = 0; i < 8; ++i) {
                header.p_len = (header.p_len << 8) | data[2 + i];
            }
            header_size += 8;
        }
        // 解析掩码密钥 (如果存在)
        if (header.mask) {
            // 需要额外 4 个字节作为掩码
            if (len < header_size + 4) {
                return false;
            }
            header_size += 4;
        }
        // 检查整个帧头是否已完全接收
        if (len < header_size) {
            return false;
        }
        header.h_len = header_size;
        return true;
    }

    static std::vector<uint8_t> EncodeWebSocketFrame(const char* payload, size_t payload_length, uint8_t is_text = 1)
    {
        std::vector<uint8_t> frame;
        // 第一个字节 (FIN + RSV1-3 + Opcode)
        uint8_t first_byte = 0x80; // FIN = 1 (表示这是消息的最后一帧)
        // RSV1, RSV2, RSV3 都为 0
        first_byte |= is_text;
        frame.push_back(first_byte);
        // 第二个字节 (Mask + Payload length)
        uint8_t  second_byte    = 0x80; // Mask = 1 (客户端发送的帧必须进行掩码处理)
        //uint64_t payload_length = payload.size();
        if (payload_length <= 125) {
            second_byte |= static_cast<uint8_t>(payload_length);
            frame.push_back(second_byte);
        } else if (payload_length <= 65535) {
            second_byte |= 126; // 126 表示后续2个字节是 payload 长度
            frame.push_back(second_byte);
            auto len_bytes = ToBigEndian(static_cast<uint16_t>(payload_length));
            frame.insert(frame.end(), len_bytes.begin(), len_bytes.end());
        } else {
            second_byte |= 127; // 127 表示后续8个字节是 payload 长度
            frame.push_back(second_byte);
            auto len_bytes = ToBigEndian(static_cast<uint64_t> (payload_length));
            frame.insert(frame.end(), len_bytes.begin(), len_bytes.end());
        }
        // 掩码密钥 (Masking Key)
        srand((unsigned) time(NULL));
        uint8_t maskKey[4] = {static_cast<uint8_t>(rand() & 0xff), static_cast<uint8_t>(rand() & 0xff), static_cast<uint8_t>(rand() & 0xff),
                              static_cast<uint8_t>(rand() & 0xff)};
        frame.push_back(maskKey[0]);
        frame.push_back(maskKey[1]);
        frame.push_back(maskKey[2]);
        frame.push_back(maskKey[3]);

        // 载荷数据 (Payload Data)将原始 payload 与掩码进行异或运算
        for (size_t i = 0; i < payload_length; ++i) {
            //frame.push_back(payload[i] ^ static_cast<uint8_t>((masking_key >> ((i % 4) * 8)) & 0xFF));
            frame.push_back(payload[i] ^ maskKey[i%4]);
        }
        //cout << "Encoded frame size: " << frame.size() << std::endl;
        return frame;
    }
};


namespace Slic3r {
namespace GUI {
#define BORDER_COLOR_NORMAL wxColour(224, 224, 224)
#define BORDER_COLOR_FOCUS  wxColour(0, 120, 215)
class BrowserTabPanel;

class FuncThread : public wxThread
{
public:
    // 定义函数类型：接收void*参数，返回void*（兼容C风格函数指针）
    using ThreadFunc = std::function<void(void*, wxEvtHandler*, int)>;
    // 构造函数：传入函数对象和参数
    FuncThread(ThreadFunc func, void* arg, wxEvtHandler* parent, int idx = 0)
        : wxThread(wxTHREAD_DETACHED), m_func(func), m_arg(arg), m_parent(parent), id(idx)
    {}
    // 线程入口：执行传入的函数
    ExitCode Entry() override
    {
        if (m_func) {
            m_func(m_arg, m_parent, id);
        }
        return (ExitCode) 0;
    }

private:
    ThreadFunc    m_func;   // 存储传入的函数对象
    void*         m_arg;    // 函数参数
    wxEvtHandler* m_parent; // 主线程窗口（用于事件通信）
    int           id = 0;
};

class CircularImageButton : public wxControl
{
public:
    // 构造函数：父窗口、ID、图片路径、位置、大小
    CircularImageButton(wxWindow*       parent,
                        wxWindowID      id,
                        const wxBitmap& bitmap,
                        const wxPoint&  pos  = wxDefaultPosition,
                        const wxSize&   size = wxDefaultSize)
        : wxControl(parent, id, pos, size, wxBORDER_NONE), m_hover(false), m_pressed(false)
    {
        // 加载图片
        //LoadImage(imagePath);
        // 设置初始尺寸
        m_bitmap = bitmap;
        m_OnPaint = true;
        if (size == wxDefaultSize) {
            SetSize(32, 32); // 默认大小
        }
        // 绑定事件
        Bind(wxEVT_PAINT, &CircularImageButton::OnPaint, this);
        Bind(wxEVT_ENTER_WINDOW, &CircularImageButton::OnMouseEnter, this);
        Bind(wxEVT_LEAVE_WINDOW, &CircularImageButton::OnMouseLeave, this);
        Bind(wxEVT_LEFT_DOWN, &CircularImageButton::OnLeftDown, this);
        Bind(wxEVT_LEFT_UP, &CircularImageButton::OnLeftUp, this);
        //Bind(wxEVT_LEAVE_WINDOW, &CircularImageButton::OnMouseLeave, this);
    }
    void SetPaintOn(bool status) { 
        m_OnPaint = status;
        Refresh();
    }
    // 设置新图片
    /*void SetImage(const wxString& imagePath)
    {
         LoadImage(imagePath);
        Refresh();
    }*/

protected:
    // 绘制按钮
    void OnPaint(wxPaintEvent& event)
    {
        if (!m_OnPaint)
            return;
        wxPaintDC          dc(this);
        const wxSize  size   = GetSize();
        const int     radius = wxMin(size.x, size.y) / 4;
        const wxPoint center(size.x / 2, size.y / 2);
        // 绘制圆形背景
        dc.SetPen(wxPen(m_hover ? *wxLIGHT_GREY : *wxWHITE, 2));
        dc.SetBrush(wxBrush(m_pressed ? *wxLIGHT_GREY : *wxWHITE));
        dc.DrawEllipse(center.x - radius*2, center.y - radius*2, radius * 4, radius * 4);

        // 绘制图片（如果加载成功）
        if (m_bitmap.IsOk()) {
            // 计算图片绘制区域（圆形内）
            wxRect imageRect(center.x - radius , center.y - radius , radius * 2 +2, radius * 2 +2);
            dc.DrawBitmap(m_bitmap, imageRect.x, imageRect.y, false);
            //gc->DrawBitmap(m_bitmap, imageRect.x, imageRect.y, imageRect.width, imageRect.height, true);
        } else {
            // 图片加载失败时显示提示文字
            dc.SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
            dc.SetPen(*wxBLACK_PEN);
            dc.DrawText("无图片", center.x - 20, center.y - 8);
        }
    }

    // 鼠标进入事件
    void OnMouseEnter(wxMouseEvent& event)
    {
        m_hover = true;
        Refresh();
    }

    //鼠标离开事件
    void OnMouseLeave(wxMouseEvent& event)
    {
        m_hover   = false;
        m_pressed = false;
        Refresh();
    }

    // 鼠标左键按下事件
    void OnLeftDown(wxMouseEvent& event)
    {
        m_pressed = true;
        Refresh();
    }

    // 鼠标左键释放事件
    void OnLeftUp(wxMouseEvent& event)
    {
        if (!m_OnPaint)
            return;
        //m_OnPaint = false;
        m_pressed = false;
        Refresh();
        // 检查是否在按钮范围内释放鼠标，触发点击事件
        const wxSize  size = GetSize();
        const wxPoint center(size.x / 2, size.y / 2);
        const int     radius = wxMin(size.x, size.y) / 2;
        wxPoint pos = event.GetPosition();
        int     dx  = pos.x - center.x;
        int     dy  = pos.y - center.y;
        if (dx * dx + dy * dy <= radius * radius) {
            // 发送按钮点击事件
            event.SetEventObject(this);
            event.SetId(GetId());
            GetParent()->ProcessWindowEvent(event);
        }
    }

private:
    // 加载并缩放图片
   
    wxBitmap m_bitmap;  // 按钮上显示的图片
    bool     m_hover;   // 鼠标是否悬停
    bool     m_pressed; // 按钮是否被按下
    bool     m_OnPaint;
};
// 自定义文件浏览器控件
#define m_Print_url  "http://%s/printer/print/start"
#define m_Pause_url  "http://%s/printer/print/pause"
#define m_Resume_url "http://%s/printer/print/resume"
#define m_Cancel_url "http://%s/printer/print/cancel"
class FileBrowserCtrl : public wxPanel
{
#define IMAGE_DOWNLOAD_FINISH 8180
#define wx_CunstmId wxID_PRINT
#define MAXFILEIDX 6

#define JosnfileName "filename" //"path"
#define LOCAL_IMAGE_PATH "%s\\%s\\%d\\"
#define LOCAL_IMAGE_NAME "%s\\%s\\%d\\%s.png"
#define m_File_url       "http://%s/server/files/directory?path=gcodes/"
#define m_File_url1      "http://%s/server/files/gcodes/%s"
#define m_Move_url       "http://%s/server/files/move"
#define m_Upload_url     "http://%s/server/files/upload"
#define m_pngload_url    "http://%s/server/files/gcodes/.thumbs/%s-%dx%d.png"
#define m_fileinfo_url   "http://%s/server/files/metadata?filename=%s"



public:
    FileBrowserCtrl(wxWindow*      parent,
                    wxString       path,
                    wxWindowID     id   = wxID_ANY,
                    const wxPoint& pos  = wxDefaultPosition,
                    const wxSize&  size = wxDefaultSize)
        : wxPanel(parent, id, pos, size), m_AppPath(path)
    {
        // 创建图像列表（用于存储图标）
        m_imageList = new wxImageList(m_ICON_SIZE, m_ICON_SIZE, true);
        // 添加文件和文件夹图标（实际项目中可替换为自定义图标）
        //m_folderIconIdx = m_imageList->Add(wxArtProvider::GetBitmap(wxART_FOLDER, wxART_OTHER, wxSize(m_ICON_SIZE, m_ICON_SIZE)));
        m_fileIconIdx = m_imageList->Add(wxArtProvider::GetBitmap(wxART_NORMAL_FILE, wxART_OTHER, wxSize(m_ICON_SIZE, m_ICON_SIZE)));
        //m_gcodeIconIdx  = m_imageList->Add(wxArtProvider::GetBitmap(wxART_EXECUTABLE_FILE, wxART_OTHER, wxSize(m_ICON_SIZE, m_ICON_SIZE)));
        m_originalColumnNames = {_("Name"), _("Last printed"), _("Modified"), _("Size")};
        // 创建列表控件（报表视图，显示表头）
        
        btn1 = new CircularImageButton(this, 1,wxArtProvider::GetBitmap(wxART_GO_HOME, wxART_OTHER, wxSize(16, 16)),wxPoint(4,4));
        btn2 = new CircularImageButton(this, 2, wxArtProvider::GetBitmap(wxART_GO_BACK, wxART_OTHER, wxSize(16, 16)), wxPoint(84, 4));
        btn3 = new CircularImageButton(this, 3, wxArtProvider::GetBitmap(wxART_GO_UP, wxART_OTHER, wxSize(16, 16)), wxPoint(164, 4));
        btn4 = new CircularImageButton(this, 4, wxArtProvider::GetBitmap(wxART_GO_DOWN, wxART_OTHER, wxSize(16, 16)), wxPoint(244, 4));
        btn5 = new CircularImageButton(this, 5, wxArtProvider::GetBitmap(wxART_GOTO_FIRST, wxART_OTHER, wxSize(16, 16)), wxPoint(324, 4));
        //Bind(wxEVT_SIZE, &FileBrowserCtrl::OnSizeChange,this);
        Bind(wxEVT_LEFT_UP, &FileBrowserCtrl::onClickButton, this);
        wxSearchCtrl* searchCtrl = new wxSearchCtrl(this, wxID_ANY, "", wxPoint(404, 8), wxSize(280, 24), wxTE_PROCESS_ENTER);
        searchCtrl->ShowSearchButton(true); // 显示搜索图标
        searchCtrl->ShowCancelButton(true); // 显示取消图标
        
        m_searchTimer = new wxTimer(this,1);
        //m_Http_tm     = new wxTimer(this, 2);
        Bind(wxEVT_TIMER, &FileBrowserCtrl::OnSearchTimer, this);
        Bind(wxEVT_TEXT, &FileBrowserCtrl::OnSearchText, this);
        wxSize size1 = wxSize(size.x, size.y - 60);
        m_listCtrl = new wxListCtrl(this, wxID_ANY, wxPoint(0,40), size1, wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES);
        //m_listCtrl   = new wxListCtrl(this, wxID_ANY, wxPoint(0, 40), size1,wxLC_HRULES);
        m_listCtrl->SetImageList(m_imageList, wxIMAGE_LIST_SMALL);

        // 设置列（带表头）
        m_listCtrl->InsertColumn(0, "", wxLIST_FORMAT_LEFT, m_ICON_SIZE+8);       // 图标列（无标题）
        m_listCtrl->InsertColumn(1, "Name", wxLIST_FORMAT_LEFT, 200);         // 名称列
        m_listCtrl->InsertColumn(2, "Last printed", wxLIST_FORMAT_LEFT, 200); // 最后打印时间列
        m_listCtrl->InsertColumn(3, "Modified", wxLIST_FORMAT_LEFT, 200);     // 修改时间列
        m_listCtrl->InsertColumn(4, "Size", wxLIST_FORMAT_RIGHT, 100);         // 大小列
        m_listCtrl->InsertColumn(5, "", wxLIST_FORMAT_RIGHT, 0);  //ImgIdx
        //m_listCtrl->SetColumnWidth(5, 0);
        // 绑定事件wxEVT_LIST_COL_RIGHT_CLICK
        m_listCtrl->Bind(wxEVT_LIST_ITEM_RIGHT_CLICK, &FileBrowserCtrl::OnRightClick, this);
        //m_listCtrl->Bind(wxEVT_RIGHT_DOWN, &FileBrowserCtrl::OnRightClick, this);
        m_listCtrl->Bind(wxEVT_LIST_ITEM_ACTIVATED, &FileBrowserCtrl::OnItemActivated, this);
        m_listCtrl->Bind(wxEVT_LIST_COL_CLICK, &FileBrowserCtrl::OnColumnClick, this);
        //Bind(wxEVT_PAINT, &FileBrowserCtrl::OnPaint, this);
        //if (hostIp != "")
        //m_Http_tm->Start(10000);
            //PopulateSampleData();
        Bind(wxEVT_COMMAND_TEXT_UPDATED, &FileBrowserCtrl::onCustomTextUpdate, this, IMAGE_DOWNLOAD_FINISH);
    }

    static int GetGcodeIcon(wxString localPath,wxString name, wxString m_ip, int m_size, wxImageList* mlist)
    {
        int ImgIdx = 0;        
        wxFileName     fileName(name);
        //wxString       file_name = wxString::FromUTF8(fileName.GetName().c_str());
        wxString       file_name = fileName.GetName();
        
        if (fileName.GetExt().Lower() != "gcode")
            return 0;
        wxStringTokenizer tokenizer(m_ip, ".");
        wxString          tmp = "unknow";
        while (tokenizer.HasMoreTokens()) {
            tmp = tokenizer.GetNextToken();
        }
        wxString LocalFileName = wxString::Format(LOCAL_IMAGE_NAME, localPath, tmp, m_size, file_name);
        wxImage  image(LocalFileName, wxBITMAP_TYPE_ANY);
        if (!image.IsOk()) {
            wxString       url = wxString::Format(m_pngload_url, m_ip, file_name, m_size, m_size);
            wxHTTP         http;
            wxInputStream* stream = HttpJsonClient::InitHttpConnection(&http, url);
            if (stream) {
                wxImage image(*stream, wxBITMAP_TYPE_PNG);
                if (image.IsOk()) {
                    image.SaveFile(LocalFileName);
                    wxBitmap bitmap(image);
                    ImgIdx = mlist->Add(bitmap);
                }
                delete stream;
            }
            http.Close();
        } 
       else {
            wxBitmap bitmap(image);
            ImgIdx = mlist->Add(bitmap);
        }
        return ImgIdx;
    }
    // 添加文件项到列表
    static wxString getFileMetadata(wxString name, wxString m_ip, int fun1 = 0 )
    { 
        wxString  url = wxString::Format(m_fileinfo_url, m_ip, name);
        pt::ptree file_info;
        if (!HttpJsonClient::ReadTree(url, file_info)) {
            return "";
        }
        try{
            pt::ptree  result     = file_info.get_child("result");           
            if (fun1 == 0) {
                boost::optional<double> opt_start_time = result.get_optional<double>("print_start_time");
                if (opt_start_time) {
                    //double     start_time = result.get<double>("print_start_time");
                    double     start_time = *opt_start_time;
                    time_t     t_time     = static_cast<time_t>(start_time);
                    wxDateTime dt(t_time);
                    return dt.Format("%Y-%m-%d %H:%M:%S");
                }
            }
            else if (fun1 == 1) {
                std::ostringstream oss;
                pt::write_json(oss, result, true); // 第三个参数为false表示不展开为多行
                return wxString::FromUTF8(oss.str());    
            }
        } 
        catch (std::exception const& e) {
            std::cerr << "Read Json error: " << e.what() << std::endl;
        }
        return "";
    
    }
    
    long AddFileItem(const wxString& name, bool isFolder, const wxString& lastPrinted, const wxString& modified, const wxString& size,int ImgIdx)
    {
        long itemIdx = m_listCtrl->InsertItem(m_listCtrl->GetItemCount(), "");
        if (ImgIdx >= 0)
            m_listCtrl->SetItemImage(itemIdx,ImgIdx);
        // 设置列数据
        m_listCtrl->SetItem(itemIdx, 1, name);        // 名称
        m_listCtrl->SetItem(itemIdx, 2, lastPrinted); // 最后打印时间
        m_listCtrl->SetItem(itemIdx, 3, modified);    // 修改时间
        m_listCtrl->SetItem(itemIdx, 4, size);        // 大小
        m_listCtrl->SetItem(itemIdx, 5, wxString::Format("%d", ImgIdx));
        // 存储完整路径（实际应用中使用）
        //m_listCtrl->SetItemData(itemIdx, wxPtrToUInt(new wxString(name)));
        return itemIdx;
    }

    void SetHostIp(wxString ip)
    {
        hostIp = ip;        
        PopulateSampleData();
    }
    // 格式化文件大小：转换为KB并添加千位分隔符
    wxString FormatFileSize(long bytes)
    {
        long kbytes = (bytes + 1023) / 1024; // +512实现四舍五入
        // 使用wxNumberFormatter添加千位分隔符
        wxString formatted = wxNumberFormatter::ToString(kbytes);
        return formatted + _("k");
    }

private:
    CircularImageButton*  btn1;
    CircularImageButton*  btn2;
    CircularImageButton*  btn3;
    CircularImageButton*  btn4;
    CircularImageButton*  btn5;
    wxListCtrl*  m_listCtrl;
    wxImageList* m_imageList;
    int m_fileIconIdx;
    int          m_fileselected = -1;
    std::vector<wxString> m_originalColumnNames; // 存储原始列名
    int                   m_sortColumn;          // 当前排序列
    bool                  m_sortAscending;       // 排序方向
    wxTimer*              m_searchTimer = nullptr;
    //wxTimer*              m_Http_tm;
    wxString              m_lastQuery;
    wxString m_AppPath;
    /*wxString              m_File_url = "http://%s/server/files/directory?path=gcodes/";
    wxString              m_File_url1 = "http://%s/server/files/gcodes/%s";
    wxString              m_Move_url = "http://%s/server/files/move";
    wxString              m_Upload_url = "http://%s/server/files/upload";
    wxString              m_pngload_url = "http://%s/server/files/gcodes/.thumbs/%s-%dx%d.png";
    wxString              m_fileinfo_url = "http://%s/server/files/metadata?filename=%s";
    wxString              m_Print_url    = "http://%s/printer/print/start";*/
    //
   // 
    wxString              hostIp     = "";
    //pt::ptree             file_root;
    json                  file_root;
    int                   m_ICON_SIZE = 32;
    int                   ICON_SIZE_idx = 0;
    //bool                  IsonPaint     = false;
    bool                  Switch_icon_b = false;
    
    void onCustomTextUpdate(wxCommandEvent& event){
        if (event.GetId() == IMAGE_DOWNLOAD_FINISH) {
            int      id    = event.GetInt();
            //wxObject* argobj = event.GetEventObject();
            json* file = (json*) event.GetEventObject();
            if (file != nullptr) {
                try {
                    AddFileItem(wxString::FromUTF8((*file)[JosnfileName]), false, wxString::FromUTF8((*file)["lpr"]),
                                wxString::FromUTF8((*file)["mod"]), FormatFileSize((*file)["size"]), (*file)["imgkey"]);
                    file_root.push_back(*file);
                    //m_listCtrl->Refresh();
                } catch (std::exception const& e) {
                    std::cerr << "Read Json error: " << e.what() << std::endl;
                }
                delete file;
            }
        } else {
            cout << "unknown event id:" << event.GetId() << endl;
        }
    }

   /* void OnSizeChange(wxSizeEvent& evt) { 
        wxSize size1 = evt.GetSize();
        size1.y -= 45;
        m_listCtrl->SetSize(size1);
    }*/

    static bool CreatImagePath(wxString m_path,wxString m_ip ,int isize){
        wxStringTokenizer tokenizer(m_ip, ".");
        wxString          tmp = "unknow";
        while (tokenizer.HasMoreTokens()) {
            tmp = tokenizer.GetNextToken();
        }
        wxString directory = wxString::Format(LOCAL_IMAGE_PATH, m_path, tmp, isize);
        if (!wxDir::Exists(directory)) {
            bool created = wxDir::Make(directory, 0777, wxPATH_MKDIR_FULL);
            if (!created) {
                std::cout << "Cant creat dir:" << directory << endl;
                return false;
            }
        }
        return true;
    }
    
    void file_root_Select(wxString str = "")
    {           
        m_listCtrl->DeleteAllItems();
        // if (!CreatImagePath(hostIp, m_ICON_SIZE))
        //    return;*/
        if (file_root.is_null() || !file_root.is_array())
            return;
        try {
            for (auto& file : file_root) {
                wxString wx_name = wxString::FromUTF8(file[JosnfileName].get<string>());
                // long        size     = file["size"].get<long>();
                if (str == "" || wx_name.Lower().Find(str.Lower()) >= 0) {
                    AddFileItem(wxString::FromUTF8(file[JosnfileName]), false, wxString::FromUTF8(file["lpr"]),
                                wxString::FromUTF8(file["mod"]), FormatFileSize(file["size"]), file["imgkey"]);
  
                }
            }
        } catch (std::exception const& e) {
            std::cerr << "Read Json error: " << e.what() << std::endl;
        }

    }

    void UploadFileToMoonraker() {
        // 创建文件选择对话框
        wxFileDialog openFileDialog(this, _("Select file"), "", "",
                                    "gcode (*.gcode)|*.gcode|"
                                    "All (*.*)|*.*|"
                                    "Image (*.png;*.jpg)|*.png;*.jpg",
                                    wxFD_OPEN | wxFD_FILE_MUST_EXIST);

        // 显示对话框，如果用户点击了确定按钮
        if (openFileDialog.ShowModal() == wxID_OK) {
            wxString filePath = openFileDialog.GetPath();
            std::cout << "file:" + filePath << endl;
            HttpJsonClient::UploadFile(wxString::Format(m_Upload_url, hostIp), filePath);
        }
    }

    void Switch_icon(){
        ICON_SIZE_idx++;
        if (ICON_SIZE_idx == 1)
            m_ICON_SIZE = 48;
        else if (ICON_SIZE_idx == 2)
            m_ICON_SIZE = 93;
        else if (ICON_SIZE_idx == 3)
            m_ICON_SIZE = 300;
        else
        {
            m_ICON_SIZE   = 32;
            ICON_SIZE_idx = 0;
        }
        m_imageList->RemoveAll();
        m_imageList = new wxImageList(m_ICON_SIZE, m_ICON_SIZE, true);
        m_listCtrl->SetImageList(m_imageList, wxIMAGE_LIST_SMALL);
        m_fileIconIdx = m_imageList->Add(wxArtProvider::GetBitmap(wxART_NORMAL_FILE, wxART_OTHER, wxSize(m_ICON_SIZE, m_ICON_SIZE)));
        m_listCtrl->DeleteAllItems();
        m_listCtrl->Refresh();
        m_listCtrl->SetColumnWidth(0, m_ICON_SIZE + 8);
        std::cout << "wxART_NORMAL_FILE=" << m_fileIconIdx << endl;
        PopulateSampleData();
    }
        
    static void GetFileListInfo(void* arg, wxEvtHandler* m_parent, int id = 0)
    {
        wxString     m_ip   = *(wxString*) ((void**) arg)[0];
        int          isize  = *(int*) ((void**) arg)[1];
        wxImageList* m_List = (wxImageList*) ((void**) arg)[2];
        wxString     m_path = *(wxString*) ((void**) arg)[3];
        //json*        pfiles  = (json*) ((void**) arg)[3];
        if (!CreatImagePath(m_path,m_ip, isize)) {
            delete ((void**) arg)[0];
            delete ((void**) arg)[1];
            delete ((void**) arg)[3];
            delete arg;
            return;
        }
        wxString url  = wxString::Format(m_File_url, m_ip);
        wxString response;
        wxCommandEvent evt(wxEVT_COMMAND_TEXT_UPDATED, IMAGE_DOWNLOAD_FINISH);
        evt.SetString("filename");
        int idx = 0;        
        if (HttpJsonClient::SendGetRequest(url, response)) {            
            try {
                json sjson = json::parse(response);
                json files = sjson["result"]["files"];
                if (!files.is_null())
                {
                    for (auto& file : files) {
                        wxString    wx_name  = wxString::FromUTF8(file[JosnfileName].get<string>());
                        //long        size     = file["size"].get<long>();
                        double      modified = file["modified"].get<double>();
                        //bool        isFolder = false; //(type == "folder");
                        time_t     modified_time = static_cast<time_t>(modified);
                        wxDateTime dt(modified_time);
                        if (wx_name == "") {
                            std::cout << "Get a empty file name\n" ;
                            continue;
                        }
                        int imgidx  = 0;
                        //int itemIdx = -1;
                        wxString last_print_t = getFileMetadata(wx_name, m_ip);
                        string   imgkey       = "img" + std::to_string(isize);
                        if (file.contains("imgkey")) {
                            imgidx = file[imgkey].get<long>();
                        } else {
                            imgidx         = GetGcodeIcon(m_path,wx_name, m_ip, isize, m_List);
                            file["imgkey"] = imgidx;
                        }
                        file["lpr"] = last_print_t.ToUTF8();
                        file["mod"] = dt.Format("%Y-%m-%d %H:%M:%S").ToUTF8();
                        //cout << &file << file << endl;
                        json* p_file = new json(file);
                        //std::unique_ptr<json> p_file = std::make_unique<json>(file);
                        //cout << p_file << *p_file << endl;
                        evt.SetInt(idx);
                        evt.SetEventObject((wxObject*) p_file);
                        //evt.SetEventObject(static_cast<wxObject*>(p_file.release()));
                        wxPostEvent(m_parent, evt);
                        idx++;
                    }
                }
              
            } catch (json::parse_error& e) {
                std::cerr << "JSON Parse error: " << e.what() << std::endl;
                //return;
            }
            catch (std::exception const& e) {
                std::cerr << "Read Json error: " << e.what() << std::endl;
                //return;
            }
        }
        delete ((void**) arg)[0];
        delete ((void**) arg)[1];
        delete arg;
        return;
    }

    // 初始化数据
    void PopulateSampleData()
    {
        if (file_root.is_array())
            file_root.clear();
        else
            file_root = json::array();
        m_listCtrl->DeleteAllItems();
        if (hostIp != "") {
            void** arg         = new void*[4];
            arg[0]             = (void*) (new wxString(hostIp));
            arg[1]             = (void*) (new int(m_ICON_SIZE));
            arg[2]             = (void*) (m_imageList);
            arg[3]             = (void*) (new wxString(m_AppPath));
            FuncThread* thread = new FuncThread(FileBrowserCtrl::GetFileListInfo, (void*) arg, this, 0);
            thread->Run();
        }
        return;
    } 
    // 更新表头显示，添加排序指示箭头
    void UpdateColumnHeaders()
    {
        for (size_t i = 0; i < m_originalColumnNames.size(); ++i) {
            wxString headerText = m_originalColumnNames[i];
            // 只为当前排序列添加箭头指示
            if (i+1 == (size_t) m_sortColumn) {
                headerText += m_sortAscending ? _(" ↑") : _(" ↓");
            }
            //m_listCtrl->setColumnText(i, headerText);
            wxListItem col; 
            m_listCtrl->GetColumn(i+1, col);
            col.SetText(headerText);
            m_listCtrl->SetColumn(i+1, col);
        }
    }
    
    // 对列表进行排序
    void SortList()
    {
        if (m_sortColumn == -1)
            return;
        // 获取当前列表项的索引
        std::vector<long> indices;
        for (long i = 0; i < m_listCtrl->GetItemCount(); ++i) {
            indices.push_back(i);
        }
        // 定义比较函数
        auto compare = [this](long a, long b) {
            wxString textA = m_listCtrl->GetItemText(a, m_sortColumn);
            wxString textB = m_listCtrl->GetItemText(b, m_sortColumn);
            // 对于大小列（第4列，索引3），按数字比较
            if (m_sortColumn == 4) {
                long sizeA = 0, sizeB = 0;
                textA.Replace(",", "");
                textA.Truncate(textA.Length() - 1);
                textB.Replace(",", "");
                textB.Truncate(textB.Length() - 1);
                textA.ToLong(&sizeA);
                textB.ToLong(&sizeB);
                return m_sortAscending ? (sizeA < sizeB) : (sizeA > sizeB);
            }
            // 对于修改日期列（第3列，索引2），按日期比较
            else if (m_sortColumn == 3 || m_sortColumn == 2) {
                wxDateTime dtA, dtB;
                dtA.ParseFormat(textA, "%Y-%m-%d %H:%M:%S");
                dtB.ParseFormat(textB, "%Y-%m-%d %H:%M:%S");
                return m_sortAscending ? (dtA < dtB) : (dtA > dtB);
            }
            // 其他列按文本比较
            else {
                return m_sortAscending ? (textA.Cmp(textB) < 0) : (textA.Cmp(textB) > 0);
            }
        };

        // 排序索引
        std::sort(indices.begin(), indices.end(), compare);

        // 根据排序后的索引重新排列列表
        std::vector<m_FileInfo> items;
        for (long idx : indices) {
            m_FileInfo info;
            info.name        = m_listCtrl->GetItemText(idx, 1);
            info.lastPrinted = m_listCtrl->GetItemText(idx, 2);
            info.modified    = m_listCtrl->GetItemText(idx, 3);
            info.size        = m_listCtrl->GetItemText(idx, 4);
            m_listCtrl->GetItemText(idx, 5).ToLong(&info.imgidx);
            //info.imgidx      =  m_listCtrl->GetItemText(idx, 5);
            items.push_back(info);
        }
        // 清空并重新添加排序后的项目
        m_listCtrl->DeleteAllItems();
        for (const auto& item : items) {
            AddFileItem(item.name, false, item.lastPrinted, item.modified, item.size, item.imgidx);
        }
    }
   
    void onClickButton(wxMouseEvent& event) {
        //Unbind(wxEVT_LEFT_UP, &FileBrowserCtrl::onClickButton, this);
        btn1->SetPaintOn(false);
        btn2->SetPaintOn(false);
        btn3->SetPaintOn(false);
        btn4->SetPaintOn(false);
        btn5->SetPaintOn(false);
        int id = event.GetId();
        std::cout << "Button:" << id << ", clicked !" << std::endl; 
        if (id == 1) {
            //Home
            //file_root_Select();
        } else if (id == 2) {            
             Switch_icon();
        } else if (id == 3 && hostIp != "") {
            UploadFileToMoonraker();
        } else if (id == 4) {
            //Down
            HttpJsonClient::PostGocde(hostIp, "G1 X100 Y100 F3000");

        } else if (id == 5) {
            //First
        }
        //CircularImageButton* bt = dynamic_cast<CircularImageButton*>(event.GetEventObject());
        //bt->SetPaintOn(true);
        btn1->SetPaintOn(true);
        btn2->SetPaintOn(true);
        btn3->SetPaintOn(true);
        btn4->SetPaintOn(true);
        btn5->SetPaintOn(true);
        //Bind(wxEVT_LEFT_UP, &FileBrowserCtrl::onClickButton, this);
    }

    void OnSearchTimer(wxTimerEvent& event)
    {
        int id = event.GetId();
        std::cout << "Timer:" << id << "run" << endl;
        if (id == 1) {
            m_searchTimer->Stop(); // 停止定时器
            // wxString query = wxString::FromUTF8(m_lastQuery.ToStdString());
            //std::cout << "Performing search for: " << m_lastQuery << std::endl;
            file_root_Select(m_lastQuery); 

        }
        //if (id == 2) {
        //    m_Http_tm->Stop();
        //    if (!IsonPaint) {
        //        IsonPaint = true;
        //        if (hostIp != "")
        //            PopulateSampleData();
        //    }
        //    //Sleep(5000);
        //    std::cout << "Timer:" << id << "end" << endl;
        //}
    }
    
    void OnSearchText(wxCommandEvent& event)
    {
        m_lastQuery = event.GetString();
        std::cout << "Search text changed! Query: " << m_lastQuery << std::endl;
        // 每次有新输入就停止并重启定时器
        if (m_searchTimer->IsRunning()) {
            m_searchTimer->Stop();
        }
        m_searchTimer->Start(500); // 500毫秒后执行，可根据需要调整
    }

    void OnColumnClick(wxListEvent& event) { 
        int column = event.GetColumn();
        if (column == m_sortColumn) {
            // 同一列再次点击，切换排序方向
            m_sortAscending = !m_sortAscending;
        } else {
            // 不同列点击，更新排序列并默认正序
            m_sortColumn    = column;
            m_sortAscending = true;
        }
        // 更新表头显示（添加/更新排序指示）
        UpdateColumnHeaders();
        // 执行排序
        SortList();
    }

    // 右键点击事件 - 显示菜单
    void OnRightClick(wxListEvent& evt)
    {
        // 获取右键点击的项目
        //int itemIdx = m_fileselected;
        m_fileselected = evt.GetIndex();
        //int itemIdx = m_listCtrl->HitTest(evt.GetPosition(),50);
        // 选中点击的项目
        m_listCtrl->SetItemState(m_fileselected, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
        // 创建右键菜单
        wxMenu* menu = new wxMenu();
        // 添加菜单项（带图标）
        wxString name = m_listCtrl->GetItemText(m_fileselected,1);
        wxFileName wx_file(name);
        if (wx_file.GetExt().Lower() == "gcode") {
            //wxMenu* subMenu = new wxMenu();
            //subMenu->Append(wx_CunstmId + 10, wxString::FromUTF8("子菜单项1"));
            //subMenu->Append(wx_CunstmId + 11, wxString::FromUTF8("子菜单项2"));
            wxMenuItem* printItem = new wxMenuItem(menu, wx_CunstmId, wxString::FromUTF8("打印"), wxEmptyString,
                                                   wxITEM_NORMAL /*,subMenu*/);
            printItem->SetBitmap(wxArtProvider::GetBitmap(wxART_PRINT, wxART_MENU));
            //printItem->SetHelp(wxString::FromUTF8("打印选中的文件"));
            menu->Append(printItem);
            // menu->Append(wx_CunstmId, wxString::FromUTF8("打印"));    //, wxArtProvider::GetBitmap(wxART_PRINT, wxART_MENU)
            menu->Append(wx_CunstmId + 1, wxString::FromUTF8("添加到队列")); //, wxArtProvider::GetBitmap(wxART_ADD_BOOKMARK, wxART_MENU)
            menu->AppendSeparator();
            menu->Append(wx_CunstmId + 2, wxString::FromUTF8("预热")); //, wxArtProvider::GetBitmap(wxART_GO_UP, wxART_MENU)
            wxMenuItem* printItem1 = new wxMenuItem(menu, wx_CunstmId+3, wxString::FromUTF8("查看详情"), wxEmptyString,wxITEM_NORMAL);
            printItem1->SetBitmap(wxArtProvider::GetBitmap(wxART_EDIT, wxART_MENU));
            menu->Append(printItem1);
        }

        menu->AppendSeparator();
        menu->Append(wx_CunstmId + 4, wxString::FromUTF8("刷新元数据"));//, wxArtProvider::GetBitmap(wxART_REFRESH, wxART_MENU));
        menu->Append(wx_CunstmId + 5, wxString::FromUTF8("预览G代码"));//, wxArtProvider::GetBitmap(wxART_VIEW_DETAILS, wxART_MENU));
        menu->AppendSeparator();
        menu->Append(wx_CunstmId + 6, wxString::FromUTF8("创建ZIP存档"));//, wxArtProvider::GetBitmap(wxART_FOLDER, wxART_MENU));
        menu->Append(wx_CunstmId + 7, wxString::FromUTF8("下载")); //, wxArtProvider::GetBitmap(wxART_FILE_SAVE_AS, wxART_MENU)
        menu->Append(wx_CunstmId + 8, wxString::FromUTF8("重命名"));
        menu->Append(wx_CunstmId + 9, wxString::FromUTF8("删除"));
        // 绑定菜单项事件
        menu->Bind(wxEVT_MENU, &FileBrowserCtrl::OnMenuPrint,   this, wx_CunstmId);
        menu->Bind(wxEVT_MENU, &FileBrowserCtrl::OnMenuEdit,    this, wx_CunstmId + 3);
        menu->Bind(wxEVT_MENU, &FileBrowserCtrl::OnMenuDownload,this, wx_CunstmId + 7);
        menu->Bind(wxEVT_MENU, &FileBrowserCtrl::OnMenuRename,  this, wx_CunstmId + 8);
        menu->Bind(wxEVT_MENU, &FileBrowserCtrl::OnMenuDelete,  this, wx_CunstmId + 9);
        std::cout << "FileBrowserCtrl::right click" << std::endl;
        // 显示菜单
        //PopupMenu(menu, evt.GetPosition());
        wxPoint screenPos = wxGetMousePosition();
        PopupMenu(menu, this->ScreenToClient(screenPos));
        delete menu;
    }

    // 双击项目事件
    void OnItemActivated(wxListEvent& evt)
    {
        m_fileselected    = evt.GetIndex();
        wxString fileName = m_listCtrl->GetItemText(evt.GetIndex(), 1);
        wxMessageBox(wxString::FromUTF8("打开文件: ") + fileName, wxString::FromUTF8("文件操作"));
    }

    // 菜单事件处理函数
    void OnMenuPrint(wxCommandEvent& evt)
    {
        wxString fileName = m_listCtrl->GetItemText(m_fileselected, 1);
        //wxMessageBox(wxString::FromUTF8("正在打印: ") + fileName, wxString::FromUTF8("打印操作"));
        std::cout << wxString::FromUTF8("正在打印: ") + fileName << endl;
        wxString url = wxString::Format(m_Print_url, hostIp);
        wxString result;
        wxString poststr = wxString::Format("{\"filename\": \"%s\"}",fileName);
        if (HttpJsonClient::SendPostRequest(url, poststr, "application/json", result)) {
            cout << "OK " ;
        } else
            cout << "eror ";
        cout << result << endl;
    }

    void OnMenuEdit(wxCommandEvent& evt)
    {
        wxString fileName = m_listCtrl->GetItemText(m_fileselected, 1);
        wxString info     = getFileMetadata(fileName, hostIp,1);
        info.Replace(wxT("\\"), wxT(""));
        wxMessageDialog dialog(this, info, wxString::FromUTF8("详细信息,copy"), wxOK | wxCANCEL);
        //dialog.ShowModal();
        if (dialog.ShowModal() == wxID_OK) {
            wxClipboard* clipboard = wxClipboard::Get();
            if (clipboard->Open()) {
                clipboard->Clear();
                clipboard->SetData(new wxTextDataObject(info));
                clipboard->Close();
            }
        }
    }

    void OnMenuDownload(wxCommandEvent& evt)//m_listCtrl->GetFirstSelected()
    {
        wxString fileName = m_listCtrl->GetItemText(m_fileselected, 1);
        //(wxString::FromUTF8("下载文件: ") + fileName, wxString::FromUTF8("下载操作"));
        wxString       url = wxString::Format(m_File_url1, hostIp, fileName);
        wxHTTP         http;
        wxInputStream* stream = HttpJsonClient::InitHttpConnection(&http, url);
        if (stream) {
            // 弹出文件保存对话框
            wxFileDialog saveDialog(this,
                                    _("Select path"),                           // 对话框标题
                                    wxEmptyString,                               // 默认路径（为空则使用当前工作目录）
                                    fileName,                                    // 默认文件名
                                    "图片文件 (*.png)|*.png|所有文件 (*.*)|*.*", // 文件过滤器
                                    wxFD_SAVE | wxFD_OVERWRITE_PROMPT            // 保存模式 + 覆盖提示
            );
            if (saveDialog.ShowModal() == wxID_OK) {
                // 获取用户选择的保存路径
                wxString savePath = saveDialog.GetPath();
                std::cout << "path:" << savePath << endl;
                HttpJsonClient::InputStreamToFile(*stream, savePath);
            }
            delete stream;
        }
        http.Close();

    }

    void OnMenuRename(wxCommandEvent& evt) { 
        wxString fileName = m_listCtrl->GetItemText(m_fileselected, 1);
        // 创建文本输入对话框
        wxTextEntryDialog dialog(this, "ReName File:",  "Input", fileName);
        // 显示对话框并检查用户是否点击了确定按钮
        if (dialog.ShowModal() == wxID_OK) {
            // 获取用户输入的文本
            wxString userInput = dialog.GetValue();
            wxString url       = wxString::Format(m_Move_url, hostIp);
            wxString postBase  = "{\"source\": \"gcodes/%s\",\"dest\": \"gcodes/%s\"}";
            wxString postData  = wxString::Format(postBase, fileName, userInput);
            wxString response;
            if (HttpJsonClient::SendPostRequest(url, postData, "application/json", response)) {
                std::cout << "Response: " << response << std::endl;
                m_listCtrl->SetItem(m_fileselected, 1, userInput);
            } 
        } 

    }
    
    void OnMenuDelete(wxCommandEvent& evt) { 
        wxString fileName = m_listCtrl->GetItemText(m_fileselected, 1);
        wxString url      = wxString::Format(m_File_url1, hostIp,fileName);
        wxString result;
        std::cout << "delete:" << url << endl;
        if (HttpJsonClient::SendGetRequest(url, result, 3, "DELETE")) {
            m_listCtrl->DeleteItem(m_fileselected);
        } else
            std::cout << "delete:" << result << endl;
    }
   
    // 清理资源
    ~FileBrowserCtrl()
    {
        // 释放存储的文件名数据
        for (long i = 0; i < m_listCtrl->GetItemCount(); ++i) {
            wxString* fileName = (wxString*) m_listCtrl->GetItemData(i);
            delete fileName;
        }
        delete m_imageList;
    }
};

class ItemPanel : public wxPanel
{
public:
    ItemPanel(wxWindow* parent,
              wxPoint   pos,
              wxSize    size,
              wxString  title,
              const string iconOn,
              const string icondefult,
              int       iconsize = 24,
              int          LayoutType = 0, // 0 竖直布局()，1 水平布局，2 自定义布��
              wxPoint   iconpt   = wxDefaultPosition,
              wxPoint   txtpos   = wxDefaultPosition,
              wxString     tooltip  = "") 
        : wxPanel(parent, wxID_ANY, pos, size), iconSzie(iconsize), m_title(title)
    {
        SetBackgroundColour(m_backcolor);
        Bind(wxEVT_PAINT, &ItemPanel::On_Paint, this);
        Bind(wxEVT_ENTER_WINDOW, &ItemPanel::On_MouseEnter, this);
        Bind(wxEVT_LEAVE_WINDOW, &ItemPanel::On_MouseLeave, this);
        //Bind(wxEVT_SIZE, &ItemPanel::On_SizeChange, this);
        if (iconOn != "")
            iconMap[1] = ScalableBitmap(this, iconOn, iconsize);
        if (icondefult != "")
            iconMap[0] = ScalableBitmap(this, icondefult, iconsize);
        Ishorizontal = LayoutType;
        if (Ishorizontal == 2) {
            m_titlePos = txtpos;
            m_iconPos  = iconpt;
        }
        if (tooltip != "") {
            m_tooltip = new wxToolTip(tooltip);
            SetToolTip(m_tooltip);
        }
        BindEventsToChildren(this);
    }

    void BindEventsToChildren(wxWindow* parent)
    {
        if (!parent)
            return;
        parent->Bind(wxEVT_ENTER_WINDOW, &ItemPanel::On_MouseEnter, this);
        parent->Bind(wxEVT_LEAVE_WINDOW, &ItemPanel::On_MouseLeave, this);
        wxWindowList& children = parent->GetChildren();
        for (wxWindowList::iterator it = children.begin(); it != children.end(); ++it) {
            BindEventsToChildren(*it);
        }
    }
    void SetToolTipText(wxString tooltip)
    {
        if (m_tooltip) {
            m_tooltip->SetTip(tooltip);
        } else {
            m_tooltip = new wxToolTip(tooltip);
            SetToolTip(m_tooltip);
        }
    }
    #if 0
    void SetIconPos(wxPoint pos)
    {
        m_iconPos = pos;
    }
    void SetTitelPos(wxPoint pos)
    {
        m_titlePos = pos;
    }
    #endif
    void SetTitle(wxString title)
    {
        m_title = title;
        Refresh();
    }
    void SetIconIdx(int index) { 
        iidx = index;
    }
    void SetTextColor(wxColour color) { txtColor = color; }
    void SetBackColor(wxColour color)
    {
        m_backcolor = color;
        SetBackgroundColour(m_backcolor);
    }
    void SetBorderEnable(bool enable) { m_Bordon = enable; }
    void setHorizontal(bool horizontal) { Ishorizontal = horizontal; }
    
    wxColour m_backcolor  = *wxWHITE;
    wxColour txtColor     = wxColor("#B0B0B0");
    bool     m_isHovering = false;
    wxPoint  m_titlePos;
    wxPoint  m_iconPos;
    wxString m_title      = "";
    int      iidx         = 0;
    bool     m_PaintClear = true;
private:
    ScalableBitmap iconMap[2];
    int            layoutType   = 0;
    int            iconSzie;
    bool           Ishorizontal = false;
    bool           m_Bordon     = false;
    wxToolTip*     m_tooltip    = nullptr;

    void InitPos(wxSize size1) { ; }
    //void On_SizeChange(wxSizeEvent& event) { InitPos(event.GetSize()); }
    void On_MouseEnter(wxMouseEvent& event)
    {
        m_isHovering = true;
        Refresh();    // 触发重绘
        event.Skip(); // 继续传播事件
    }
    void On_MouseLeave(wxMouseEvent& event)
    {
        m_isHovering = false;
        Refresh();    // 触发重绘
        event.Skip(); // 继续传播事件
    }
    void On_Paint(wxPaintEvent& event)
    {
        wxPaintDC dc(this);
        if (m_PaintClear)
            dc.Clear();
        wxRect rect = GetClientRect();        
        dc.SetTextForeground(txtColor);
        dc.SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        //dc.DrawText(m_title, m_titlePos.x -, m_titlePos.y);
        if (Ishorizontal == 0) { // 竖直布局
            if (iconMap[iidx].bmp().IsOk()) {
                dc.DrawBitmap(iconMap[iidx].bmp(), wxPoint((rect.width - iconSzie) / 2, (rect.width - iconSzie)/4));
            }
            if (m_title != "") {
                dc.DrawText(m_title,
                            wxPoint((rect.width - dc.GetTextExtent(m_title).GetWidth()) / 2, (rect.width - iconSzie) / 4 + iconSzie + 4));
            }
        } else if (Ishorizontal == 1) { // 水平布局
            if (iconMap[iidx].bmp().IsOk()) {
                //dc.DrawBitmap(iconMap[iidx].bmp(), wxPoint(4, rect.height / 2 - iconSzie / 2));
                dc.DrawBitmap(iconMap[iidx].bmp(), wxPoint((rect.height - iconSzie) / 4, rect.height / 2 - iconSzie / 2));
            }
            if (m_title != "")
                dc.DrawText(m_title, wxPoint((rect.height - iconSzie) / 4 + iconSzie +4, rect.height / 2 - 8));
        } else {
            if (m_title != "")
                dc.DrawText(m_title, m_titlePos);
            if (iconMap[iidx].bmp().IsOk()) {
                dc.DrawBitmap(iconMap[iidx].bmp(), m_iconPos);
            }
        }
        int m_borderWidth = 2; // 边框宽度为2像素
        //rect.Inflate(-m_borderWidth, -m_borderWidth);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        // 如果鼠标悬停，则绘制边框
        if (m_isHovering) {
            dc.SetPen(wxPen(BORDER_COLOR_FOCUS, m_borderWidth, wxPENSTYLE_SOLID));
            dc.DrawRectangle(1, 1, rect.width - 1, rect.height - 1); // 绘制边框
        } else if (m_Bordon) {
            dc.SetPen(wxPen(BORDER_COLOR_NORMAL, m_borderWidth, wxPENSTYLE_SOLID));
            dc.DrawRectangle(1, 1, rect.width - 1, rect.height - 1); // 绘制边框
        }
        event.Skip();           // 继续传播
    }
};

class ExtruderPanel : public wxPanel
{
    #if 0
#define Icon1_pos    wxPoint(6, 12)
#define Icon2_pos    wxPoint(104, 22)
#define Icon3_pos    wxPoint(120, 12)
#define Input_pos    wxPoint(70, 18)
#define Lable_pos    wxPoint(35, 22)
#define Name_pos     wxPoint(35, 3)
    #endif
public:
    ExtruderPanel(wxWindow* parent, int id,bool Temptype, wxPoint pos, wxSize size, const char* tempname,double scanx= 1.0)
        : wxPanel(parent, wxID_ANY, pos, size), m_Id(id), m_Temptype(Temptype), m_tempname(tempname)
    {
        m_timer = new wxTimer(this);
        Bind(wxEVT_TIMER, [=](wxTimerEvent& event) {
            input_text->Hide();
            cout << "timer:" << tempname << endl;
        });
        SetBackgroundColour(*wxWHITE);
        //SetMinSize(wxSize(148, 48));
        if (m_Temptype) {   //Extruder
            normal_icon  = ScalableBitmap(this, "monitor_nozzle_temp", 24);
            actice_icon  = ScalableBitmap(this, "monitor_nozzle_temp_active", 24);
            fan_on_icon  = ScalableBitmap(this, "monitor_fan_on", 24);
            fan_off_icon = ScalableBitmap(this, "monitor_fan_off", 24);
        } else {            //Bed
            actice_icon = ScalableBitmap(this, "monitor_bed_temp_active", 24);
            normal_icon = ScalableBitmap(this, "monitor_bed_temp", 24);
        }
        degree_icon       = ScalableBitmap(this, "degree", 16);


        #if 0
        m_extruder  = new wxStaticBitmap(this, wxID_ANY, normal_icon.bmp(), Icon1_pos);
        m_degree    = new wxStaticBitmap(this, wxID_ANY, degree_icon.bmp(), Icon2_pos);
        m_fan       = new wxStaticBitmap(this, wxID_ANY, fan_off_icon.bmp(), Icon3_pos);
        temp_text   = new wxStaticText(this, wxID_ANY, "", Lable_pos);
        name_text         = new wxStaticText(this, wxID_ANY, m_tempname, Name_pos);
        input_text = new wxTextCtrl(this, wxID_ANY, "", Input_pos, wxSize(35, 20), wxTE_PROCESS_ENTER, wxTextValidator(wxFILTER_NUMERIC));
        #else
        m_extruder = new wxStaticBitmap(this, wxID_ANY, normal_icon.bmp());
        m_degree   = new wxStaticBitmap(this, wxID_ANY, degree_icon.bmp());
        m_fan      = new wxStaticBitmap(this, wxID_ANY, fan_off_icon.bmp());
        temp_text  = new wxStaticText(this, wxID_ANY, "---.-/---");
        name_text  = new wxStaticText(this, wxID_ANY, m_tempname);
        input_text = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER,
                                    wxTextValidator(wxFILTER_NUMERIC));

        wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);
        wxBoxSizer* textSizer = new wxBoxSizer(wxVERTICAL);
        wxBoxSizer* tempSizer = new wxBoxSizer(wxHORIZONTAL);
        textSizer->Add(name_text, 0, wxALIGN_CENTER_HORIZONTAL);
        tempSizer->Add(temp_text,0, wxEXPAND | wxLEFT |wxBOTTOM ,10);
       // tempSizer->Add(input_text, 0, wxEXPAND);
        tempSizer->Add(m_degree, 0, wxEXPAND | wxLEFT | wxBOTTOM, 10);
        textSizer->Add(tempSizer, 0, wxEXPAND );
        mainSizer->Add(m_extruder, 0, wxEXPAND | wxALL, 10);
        mainSizer->Add(textSizer, 1, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM,10);
        mainSizer->Add(m_fan, 0, wxEXPAND | wxALL, 10);
        SetSizer(mainSizer);
        #endif
        input_text->Hide();
        //UpdateTempText();
        input_text->Bind(wxEVT_KILL_FOCUS, [=](wxFocusEvent& event) {
            if (m_timer->IsRunning()) {
                m_timer->Stop();
            }
            input_text->Hide();
            double tmp_target;
            input_text->GetValue().ToDouble(&tmp_target);
            if (t_target != tmp_target) {
                SetTarget(tmp_target);
            }
            //UpdateTempText();
        });
        input_text->Bind(wxEVT_TEXT_ENTER, [=](wxCommandEvent& event) {
            if (m_timer->IsRunning()) {
                m_timer->Stop();
            }
            input_text->Hide();
        });
        input_text->Bind(wxEVT_TEXT, [=](wxCommandEvent& event) {
            if (m_timer->IsRunning()) {m_timer->Stop();}
            m_timer->Start(2000,true); // 2秒后触发
        });
          
        if (!m_Temptype) {
            m_fan->Hide();
        }
        BindEventsToChildren(this);

        m_fan->Bind(wxEVT_LEFT_DOWN, &ExtruderPanel::OnLeftDown, this);
        m_degree->Bind(wxEVT_LEFT_DOWN, &ExtruderPanel::OnLeftDown, this);
        m_extruder->Bind(wxEVT_LEFT_DOWN, &ExtruderPanel::OnLeftDown, this);

        Bind(wxEVT_PAINT, &ExtruderPanel::OnPaint, this);
        //Bind(wxEVT_BUTTON, &TempInputButton::OnClick, this);
        Bind(wxEVT_LEFT_DOWN, &ExtruderPanel::OnLeftDown, this);
        Bind(wxEVT_ENTER_WINDOW, &ExtruderPanel::OnMouseEnter, this);
        Bind(wxEVT_LEAVE_WINDOW, &ExtruderPanel::OnMouseLeave, this);        
        
        temp_text->Bind(wxEVT_LEFT_DOWN, &ExtruderPanel::OnLeftDown, this);
        name_text->Bind(wxEVT_LEFT_DOWN, &ExtruderPanel::OnLeftDown, this);
    }
    void BindEventsToChildren(wxWindow* parent)
    {
        if (!parent)
            return;
        parent->Bind(wxEVT_ENTER_WINDOW, &ExtruderPanel::OnMouseEnter, this);
        parent->Bind(wxEVT_LEAVE_WINDOW, &ExtruderPanel::OnMouseLeave, this);
        wxWindowList& children = parent->GetChildren();
        for (wxWindowList::iterator it = children.begin(); it != children.end(); ++it) {
            BindEventsToChildren(*it);
        }
    }
    void UpdateTempText()
    { 
        if (input_text->IsShown())
            return;
        wxString kongge = "";
        if (f_temp < 100)
            kongge += " ";
        if (f_temp < 10)
            kongge += " ";
        if (t_target == floor(t_target))
            kongge += wxString::Format("%.1f/%.0f", f_temp, t_target);
        else
            kongge += wxString::Format("%.1f/%.1f", f_temp, t_target);
        temp_text->SetLabelText(kongge);
    }
    void SetTarget(double tmp)
    {
        //$ SET_HEATER_TEMPERATURE HEATER=extruder3 TARGET=30
        t_target               = tmp;
        wxString command = wxString::Format("SET_HEATER_TEMPERATURE HEATER=%s TARGET=%.1f", m_tempname, t_target);
        wxCommandEvent evt(wxEVT_BUTTON);
        evt.SetInt(810);
        evt.SetString(command);
        ProcessWindowEvent(evt);
        Refresh();
    }
    void UpdateType(bool Type)
    {
        m_Temptype = Type;
        Refresh();
    }
    void UpdateTarget(double temp)
    {
        t_target = temp;
        UpdateTempText();
        //Refresh();
    }
    void SetTempFan(bool ison)
    {
        m_isFanOn = ison;
        //Refresh();
        if (m_isFanOn) {
            m_fan->SetBitmap(fan_on_icon.bmp());
        } else {
            m_fan->SetBitmap(fan_off_icon.bmp());
        }

    }
    void SetTemp(double temp)
    {
        f_temp = temp;
        UpdateTempText();
        //Refresh();
    }
    void SetActive(double power)
    {
        if (power > 0)
            m_isActive = true;
        else
            m_isActive = false;
        //Refresh();
        if (m_isActive) {
            m_extruder->SetBitmap(actice_icon.bmp());
        } else {
            m_extruder->SetBitmap(normal_icon.bmp());
        }
    }
    void SetFilamentColor(wxColour color)
    {
        m_FilamentSet = true;
        name_text->Unbind(wxEVT_LEFT_DOWN, &ExtruderPanel::OnLeftDown, this);
        name_text->Bind(wxEVT_LEFT_DOWN, &ExtruderPanel::ShowColourDialog, this);
        m_filament = color;
        name_text->SetForegroundColour(m_filament);
        if (m_filament.Green() > 160) {
            name_text->SetBackgroundColour(*wxBLACK);
        } else
            name_text->SetBackgroundColour(*wxWHITE);
        Refresh();
    }
    
private:
    int            m_Id;
    bool           m_Temptype;
    bool           m_FilamentSet = false;
    const char*    m_tempname;
    bool           m_isHovering = false;
    bool           m_isActive   = false;
    bool           m_isFanOn    = false;
    int            m_enterCount = 0;
    wxTimer*       m_timer;
    ScalableBitmap normal_icon;
    ScalableBitmap actice_icon;
    ScalableBitmap degree_icon;
    ScalableBitmap fan_on_icon;
    ScalableBitmap fan_off_icon;
    double         f_temp   = 25.0f;
    double         t_target = 0;
    wxStaticText*  temp_text;
    wxStaticText*  name_text;
    wxStaticBitmap* m_extruder;
    wxStaticBitmap* m_degree;
    wxStaticBitmap* m_fan;
    wxTextCtrl*     input_text;
    wxColour    m_filament = wxColour(128,128,128);
    void           ShowColourDialog(wxMouseEvent& event)
    {
        wxColourData data;
        data.SetChooseFull(true);              // 允许选择全范围的颜色
        data.SetColour(GetBackgroundColour()); // 设置初始颜色
        wxColourDialog dialog(this, &data);    // 创建并显示颜色选择对话框
        if (dialog.ShowModal() == wxID_OK) {
            //m_filament = dialog.GetColourData().GetColour();
            SetFilamentColor(dialog.GetColourData().GetColour());
            // 更新面板的背景颜色
            wxCommandEvent evt(wxEVT_BUTTON);
            // 用户点击了确定按钮，获取选中的颜色
            evt.SetString(wxString::Format("SET_LED LED=T%d_RGB GREEN=%.4f RED=%.4f BLUE=%.4f", m_Id, 1.0 * m_filament.Green() / 255,
                                                   1.0 * m_filament.Red() / 255, 1.0 * m_filament.Blue() / 255));
            evt.SetInt(810);
            GetParent()->ProcessWindowEvent(evt);
            // SetColor(m_color);
            //Refresh();
        }
    }
    void OnLeftDown(wxMouseEvent& event)
    {
        wxPoint pos = event.GetPosition();
        //if (m_FilamentSet && pos.y < 20 && pos.x > 30 && pos.y < 100) {
        //    ShowColourDialog();
        //} else {
            if (input_text->IsShown()) {
                input_text->Hide();
            } else {
                m_timer->Start(5000, true); // 2秒后触发
                wxPoint pt;
                wxSize  size1 = this->GetSize();
                pt.x          = size1.x / 2;
                pt.y          = size1.y / 2 - 4;
                size1.x       = size1.x /4; 
                size1.y       = pt.y;
                input_text->SetSize(size1);
                input_text->SetPosition(pt);
                input_text->Show();
                input_text->SetFocus();
                input_text->SetSelection(-1, -1); // 全选文本
            }
//}
    }
    void OnMouseEnter(wxMouseEvent& event)
    {
        m_enterCount++;
        if (!m_isHovering && m_enterCount == 1) {
            m_isHovering = true;
            Refresh(); // 触发重绘
        }
        event.Skip(); // 继续传播事件
    }
    void OnMouseLeave(wxMouseEvent& event)
    {
        m_enterCount--;
        if (m_isHovering && m_enterCount == 0) {
            m_isHovering = false;
            Refresh(); // 触发重绘
        }
        event.Skip(); // 继续传播事件
    }
    void OnPaint(wxPaintEvent& event)
    {
        wxPaintDC dc(this);
        dc.Clear();
        #if 0
        if (m_Temptype) {
            
            // dc.DrawRectangle(wxRect(0,0,36,48));
        }
        if (m_isActive) {
            if (actice_icon.bmp().IsOk()) {
                dc.DrawBitmap(actice_icon.bmp(), Icon1_pos, true);
            }
        } else {
            if (normal_icon.bmp().IsOk()) {
                dc.DrawBitmap(normal_icon.bmp(), Icon1_pos, true);
            }
        }        
        if (degree_icon.bmp().IsOk()) {
            dc.DrawBitmap(degree_icon.bmp(), Icon2_pos, true);
        }
        
        if (m_tempname != nullptr && m_Id < 5) {
            //dc.SetTextForeground(wxColour("#808080"));
            dc.SetTextForeground(m_filament);
            if (m_filament.Green() > 160) {
                //dc.SetTextBackground(*wxBLACK);
                int textWidth, textHeight;
                dc.GetTextExtent(m_tempname, &textWidth, &textHeight);
                dc.SetPen(*wxBLACK_PEN);
                dc.SetBrush(*wxBLACK_BRUSH);
                dc.DrawRectangle(Name_pos.x, Name_pos.y, textWidth, textHeight);
            }
            dc.DrawText(m_tempname, Name_pos);
        }
        
        wxString kongge = "";
        if (f_temp < 100)
            kongge += " ";
        if (f_temp < 10)
            kongge += " ";
        dc.SetTextForeground(*wxBLACK);
        if (t_target == floor(t_target))
            dc.DrawText(wxString::Format("%s%.1f/%.0f", kongge, f_temp, t_target), Lable_pos);
        else
            dc.DrawText(wxString::Format("%s%.1f/%.1f", kongge, f_temp, t_target), Lable_pos);
                
        if (m_Temptype) {
            /*dc.SetPen(wxPen(m_filament, 3));
            dc.DrawLine(wxPoint(17, 1), wxPoint(17, 18));
            dc.DrawLine(wxPoint(17, 34), wxPoint(17, 40));*/
            if (m_isFanOn) {
                if (fan_on_icon.bmp().IsOk()) {
                    dc.DrawBitmap(fan_on_icon.bmp(), Icon3_pos, true);
                }
            } else {
                if (fan_off_icon.bmp().IsOk()) {
                    dc.DrawBitmap(fan_off_icon.bmp(), Icon3_pos, true);
                }
            }

        #endif
        wxRect rect = GetClientRect();
        int    m_borderWidth = 2; // 边框宽度为2像素
        rect.Inflate(-m_borderWidth, -m_borderWidth);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        // 如果鼠标悬停，则绘制边框
        if (m_isHovering) {
            dc.SetPen(wxPen(BORDER_COLOR_FOCUS, m_borderWidth, wxPENSTYLE_SOLID));
            dc.DrawRectangle(rect); // 绘制边框
        } else
            dc.SetPen(wxPen(BORDER_COLOR_NORMAL, m_borderWidth, wxPENSTYLE_SOLID));
        
        event.Skip();           // 继续传播 
        
    }      
};

#if 0
// 线程安全的结果存储
class PrinterResult
{
public:
    static PrinterResult& GetInstance()
    {
        static PrinterResult instance;
        return instance;
    }

    void AddIP(const wxString& ip)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_ips.push_back(ip);
    }

    std::vector<wxString> GetIPs()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_ips;
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_ips.clear();
    }

private:
    PrinterResult() = default;
    std::vector<wxString> m_ips;
    std::mutex            m_mutex;
};
#endif
class MeterPanel : public ItemPanel
{
public:
    MeterPanel(wxWindow* parent, wxStandardID id = wxID_ANY, wxPoint pos = wxDefaultPosition, wxSize size = wxDefaultSize)
        : ItemPanel(parent, pos, size, "", "","fan_dash_bk",32,1)
    {
        char tmp[32];
        //m_bitmap[11] = ScalableBitmap(this, "fan_dash_bk", 32);
        for (int i = 0; i < 11; i++) {
            snprintf(tmp, 32, "fan_scale_%d", i);
            m_bitmap[i] = ScalableBitmap(this, tmp, 32);
        }
        Bind(wxEVT_PAINT, &MeterPanel::onPaint, this);
        m_PaintClear = false;
    }
    void SetCpuUsage(float usage)
    {
        cpu_usage = usage > 100 ? 100 : usage;
        SetTitle(wxString::Format("%.0f%%", cpu_usage));
        Refresh();
    }

private:
    float cpu_usage = 0.0f;
    ScalableBitmap m_bitmap[11];
    void onPaint(wxPaintEvent& event)
    {
        wxPaintDC dc(this);
        dc.Clear();
        int idx = static_cast<int>((cpu_usage + 5) / 10.0f);
        if (idx > 10)
            idx = 10;
        wxRect rect = GetClientRect();
        //dc.DrawBitmap(m_bitmap[idx].bmp(), 4, Icon32 * 8, true);
        dc.DrawBitmap(m_bitmap[idx].bmp(), wxPoint(4, rect.height / 2 - 32 / 2));       
        event.Skip();           // 继续传播
    }
};

// 自定义进度对话框
class CustomProgressDialog : public wxDialog
{
public:
    // 构造函数：标题、父窗口、初始进度值
    CustomProgressDialog(const wxString& title, wxWindow* parent, int initialValue = 0, wxPoint pos = wxDefaultPosition)
        : wxDialog(parent, wxID_ANY, title, pos, wxSize(300, 200))
    {
        // 创建主面板
        m_value               = std::clamp(initialValue, 0, 100);
        wxPanel*    panel     = new wxPanel(this, wxID_ANY);
        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
        //// 1. 进度条（wxGauge）
        //m_gauge = new wxGauge(panel, wxID_ANY, 100, wxDefaultPosition, wxSize(-1, 10));
        //m_gauge->SetValue(m_value);
        //mainSizer->Add(m_gauge, 0, wxEXPAND | wxALL, 10);
        // 2. 滑块（wxSlider）：支持拖动调整进度
        m_slider = new wxSlider(panel, wxID_ANY, m_value, 0, 100, wxDefaultPosition, wxSize(-1, -1), wxSL_HORIZONTAL | wxSL_LABELS);
        mainSizer->Add(m_slider, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
        // 3. 数字输入框（wxTextCtrl）：支持直接输入数字
        wxBoxSizer* inputSizer = new wxBoxSizer(wxHORIZONTAL);
        inputSizer->Add(new wxStaticText(panel, wxID_ANY, "value："), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
        m_textCtrl = new wxTextCtrl(panel, wxID_ANY, wxString::Format("%d", m_value), wxDefaultPosition, wxSize(60, -1), 0,
                                    wxTextValidator(wxFILTER_NUMERIC));
        inputSizer->Add(m_textCtrl, 0, wxALIGN_CENTER_VERTICAL);
        inputSizer->Add(new wxStaticText(panel, wxID_ANY, "%"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
        mainSizer->Add(inputSizer, 0, wxALIGN_CENTER | wxALL, 10);
        // 4. 按钮（确定/取消）
        wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
        btnSizer->Add(new wxButton(panel, wxID_OK, "Ok"), 0, wxRIGHT, 10);
        btnSizer->Add(new wxButton(panel, wxID_CANCEL, "Canel"), 0);
        mainSizer->Add(btnSizer, 0, wxALIGN_CENTER | wxALL, 10);
        // 设置布局
        panel->SetSizer(mainSizer);
        mainSizer->Fit(this);
        // 绑定事件：滑块拖动或输入框修改时，同步更新所有控件
        m_slider->Bind(wxEVT_SLIDER, &CustomProgressDialog::OnSliderChange, this);
        m_textCtrl->Bind(wxEVT_TEXT, &CustomProgressDialog::OnTextChange, this);
    }
    // 获取用户设置的进度值
    int GetValue() const { return m_value; }

private:
    //wxGauge*    m_gauge;    // 进度条
    wxSlider*   m_slider;   // 滑块（拖动调整）
    wxTextCtrl* m_textCtrl; // 文本框（输入数字）
    int         m_value;    // 当前进度值

    // 滑块拖动时，同步更新进度条和输入框
    void OnSliderChange(wxCommandEvent& event)
    {
        m_value = m_slider->GetValue();
        //m_gauge->SetValue(m_value);
        m_textCtrl->SetValue(wxString::Format("%d", m_value));
    }
    // 输入框修改时，同步更新进度条和滑块
    void OnTextChange(wxCommandEvent& event)
    {
        long val;
        if (m_textCtrl->GetValue().ToLong(&val)) {
            m_value = std::clamp(static_cast<int>(val), 0, 100);
            //m_gauge->SetValue(m_value);
            m_slider->SetValue(m_value);
        }
    }
};

class FanPanel : public ItemPanel
{
public:
    FanPanel(wxWindow* parent,
             wxPoint   pos,
             wxSize    size,
             wxString  title,
             string    icon_on  = "",
             string    icon_off = "",
             int       iconsize = 32,
             bool      Dlg      = false,
             int       layout   = 0,
             wxString  tooltip  = "")
        : ItemPanel(parent, pos, size, title, icon_on, icon_off, iconsize, layout), progressDlg(Dlg)
    {
        Bind(wxEVT_LEFT_UP, &FanPanel::onClick, this);
        if (tooltip != "")
            SetToolTipText(tooltip);
        //txtColor = BORDER_COLOR_NORMAL;
    }
    void SetDlgMode(bool dlg) { progressDlg = dlg; }
    void SetLampState(int ison)
    {
        m_pwm = ison;
        if (m_pwm > 0) {
            iidx = 1;
            if (m_pwm == 100)
                m_title = "On";
            else if (m_pwm < 100)
                m_title = wxString::Format("%d%%", m_pwm);
            else
                m_title = wxString::Format("%d", m_pwm);
        } else {
            iidx    = 0;
            m_title    = "Off";
        }
        // m_Status = wxString::Format("%s", m_pwm ? "On" : "Off");
        Refresh();
    }
private:
    bool progressDlg;
    int  m_pwm = 0;
    void onClick(wxMouseEvent& event)
    {
        if (!progressDlg) {
            iidx = (iidx + 1) & 1;
        }
        else
            {
            wxPoint              pos = ClientToScreen(wxPoint((GetSize().GetWidth() - 300) / 2, (GetSize().GetHeight() - 200) / 2));
            CustomProgressDialog dlg(m_title, this, m_pwm, pos);
            if (dlg.ShowModal() == wxID_OK) {
                // 用户点击确定后，获取进度值并显示
                int value = dlg.GetValue();
                SetLampState(value);
            }
        }
        cout << "LampPanel::onClick: m_pwm=" << m_pwm << endl;
        wxCommandEvent evt(wxEVT_BUTTON);
        evt.SetInt(810);
        // if (m_pwm) {
        //     evt.SetString("M355 S1"); // 开灯
        // } else {
        //     evt.SetString("M355 S0"); // 关灯
        // }
        // GetParent()->ProcessWindowEvent(evt);
        Refresh(); // 触发重绘
    }
};

class MoveBarPanel : public wxPanel
{
public:
    MoveBarPanel(wxWindow* parent, std::string strIcon, wxStandardID id, wxPoint pos, wxSize size, double scalex =1.0)
        : wxPanel(parent, id, pos, size), m_Icon(strIcon), scaleX(scalex)
    {
        //SetBackgroundColour(*wxWHITE);
        Bind(wxEVT_PAINT, &MoveBarPanel::onPaint, this);
        Bind(wxEVT_LEFT_UP, &MoveBarPanel::onClick, this);
        Bind(wxEVT_LEFT_DOWN, &MoveBarPanel::onLeftDown, this);
        Bind(wxEVT_MOTION, &MoveBarPanel::OnMouseMove, this);
        Bind(wxEVT_LEAVE_WINDOW, &MoveBarPanel::OnMouseLeave, this);
        Bind(wxEVT_SIZE, &MoveBarPanel::OnSizeChange, this);
        Bind(wxEVT_DPI_CHANGED, &MoveBarPanel::OnDPIChange, this);
        //bitmap = ScalableBitmap(this, strIcon, 216 * 25 / 256);
        //wxDisplay mdisplay(wxDisplay::GetFromWindow(this));
        //double    scaleX = mdisplay.GetScaleFactor();
        //this->SetMinSize(wxSize(224 * scaleX, 224 * scaleX));
        InitSize(size);
        toolTip            = new wxToolTip("");
        this->SetToolTip(toolTip);
        mouseTimer = new wxTimer(this);
    }
    void SetXHomed(bool homed)
    {
        X_homed = homed;
        Refresh();
    }
    void SetBusy(bool busy)
    {
        IsBusy = busy;
        Refresh();
    }

private:
    ScalableBitmap bitmap;
    std::string    m_Icon;
    wxTimer*       mouseTimer;
    wxToolTip*     toolTip;
    double         scaleX   = 1.0;
    int            IconSize = 32;
    int            focosId  = 0;
    int            posx;
    int            posy;
    int            radius1; //   = 21;
    int            radius2; //   = 60;
    int            radius3; //   = 105;
    int            leftdown_w = 0;
    bool           X_homed    = false;
    bool           IsBusy     = false;
    int            SQUARE1;// = 16;
    int            SQUARE2; //   = 42;
    int            SQUARE3    = 77;
    int            IconoffsetX;

    void DrawArcLine(wxDC* dc, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4)
    {
        dc->DrawArc(wxPoint(posx + x1, posy + y1), wxPoint(posx + x2, posy + y2), wxPoint(posx, posy));
        dc->DrawArc(wxPoint(posx + x3, posy + y3), wxPoint(posx + x4, posy + y4), wxPoint(posx, posy));
        dc->DrawLine(posx + x1, posy + y1, posx + x3, posy + y3);
        dc->DrawLine(posx + x2, posy + y2, posx + x4, posy + y4);
    }    
    int  GetMousePosIndex(wxMouseEvent& event)
    {
        int     focusIndex = 0;
        if (IsBusy)
            return focusIndex;
        wxPoint pos        = event.GetPosition();               
        if ((pos.x - posx) * (pos.x - posx) + (pos.y - posy) * (pos.y - posy) < radius3 * radius3) {
            if ((pos.x - posx) * (pos.x - posx) + (pos.y - posy) * (pos.y - posy) < radius1 * radius1)
                focusIndex = 1;
            else {
                if (X_homed) {
                    if ((pos.x - posx) * (pos.x - posx) + (pos.y - posy) * (pos.y - posy) < radius2 * radius2) {
                        if (pos.x < posx) {
                            if ((posx - pos.x) > abs(posy - pos.y))
                                focusIndex = 2;
                            else {
                                if (pos.y < posy)
                                    focusIndex = 3;
                                else
                                    focusIndex = 5;
                            }
                        } else {
                            if ((pos.x - posx) > abs(posy - pos.y))
                                focusIndex = 4;
                            else {
                                if (pos.y < posy)
                                    focusIndex = 3;
                                else
                                    focusIndex = 5;
                            }
                        }
                    } else {
                        if (pos.x < posx) {
                            if ((posx - pos.x) > abs(posy - pos.y))
                                focusIndex = 6;
                            else {
                                if (pos.y < posy)
                                    focusIndex = 7;
                                else
                                    focusIndex = 9;
                            }
                        } else {
                            if ((pos.x - posx) > abs(posy - pos.y))
                                focusIndex = 8;
                            else {
                                if (pos.y < posy)
                                    focusIndex = 7;
                                else
                                    focusIndex = 9;
                            }
                        }
                    }
                }
            }
        } 
        return focusIndex;
    }

    wxPoint GetPointFromCirCle(wxPoint pt, int idx)
    {
        wxPoint ptResult(0, 0);
        int     timesx = 0;
        int     timesy = 0;
        if (abs(pt.x - posx) < 40) {
            if (idx < 6)
                timesx = 1;
        } else if (abs(pt.x - posx) < 50)
            timesx = 5;
        else if (abs(pt.x - posx) < 60)
            timesx = 10;
        else
            timesx = (abs(pt.x - posx) - 50) / 5 * 10;
        if (abs(pt.y - posy) < 40) {
            if (idx < 6)
                timesy = 1;
        } else if (abs(pt.y - posy) < 50)
            timesy = 5;
        else if (abs(pt.y - posy) < 60)
            timesy = 10;
        else
            timesy = (abs(pt.y - posy) - 50) / 5 * 10;

        switch (idx) {
        case 2: ptResult.x = -1; break;
        case 3: ptResult.y = 1; break;
        case 4: ptResult.x = 1; break;
        case 5: ptResult.y = -1; break;
        default:
            ptResult = wxPoint(1, 1);
            if (pt.x < posx)
                ptResult.x = -1;
            if (pt.y > posy)
                ptResult.y = -1;
            break;
        }
        ptResult.x *= timesx;
        ptResult.y *= timesy;
        return ptResult;
    }
    
    void InitSize(wxSize size1)
    {
        posx  = size1.GetWidth() / 2;
        posy  = size1.GetHeight() / 2;
        int n = size1.GetWidth();
        if (n > size1.GetHeight())
            n = size1.GetHeight();
        radius1 = n * 25 / 256;
        radius2 = n * 72 / 256;
        radius3 = n * 124 / 256;
        SQUARE1 = sqrt(radius1 * radius1 / 2);
        SQUARE2 = sqrt(radius2 * radius2 / 2);
        SQUARE3         = sqrt(radius3 * radius3 / 2);
        IconSize         = static_cast<int>(n * 45 / 512 * 2 / scaleX);
        IconoffsetX = static_cast<int>(IconSize * scaleX / 2);
        bitmap           = ScalableBitmap(this, m_Icon, IconSize);
    }

     void OnDPIChange(wxDPIChangedEvent& event)
    {
        wxDisplay mdisplay(wxDisplay::GetFromWindow(this));
        scaleX         = mdisplay.GetScaleFactor();
        cout << "scaleX:" << scaleX << endl;
        InitSize(this->GetSize());
    }

    void OnSizeChange(wxSizeEvent& event){
        wxSize size1 = event.GetSize();
        if (size1.x > 0 && size1.y > 0) {
            InitSize(size1);
            Refresh(); // 触发重绘
        }
        event.Skip();
    }
    void OnMouseMove(wxMouseEvent& event)
    {        
        int focusIndex = GetMousePosIndex(event);
        
        if (focusIndex != focosId) {
            if (mouseTimer->IsRunning())
                return;
            mouseTimer->StartOnce(100); // 100毫秒后触发
            focosId = focusIndex;
            cout << "OnMouseMove:" << focosId << endl;
            wxPoint  pos     = GetPointFromCirCle(event.GetPosition(), focusIndex);
            wxString tipText = wxString::Format("x=%d,y=%d", pos.x, pos.y);
            wxToolTip::Enable(false);
            toolTip->SetTip(tipText);
            toolTip->SetDelay(200);
            wxToolTip::Enable(true);
            Refresh(); // 触发重绘
        } else if (focusIndex > 1) {
            wxPoint  pos     = GetPointFromCirCle(event.GetPosition(), focusIndex);
            wxString tipText = wxString::Format("x=%d,y=%d", pos.x, pos.y);
            toolTip->SetTip(tipText);
            //toolTip->SetDelay(200);
            //toolTip->Enable(true);
        } else
            wxToolTip::Enable(false);
        
        event.Skip(); // 继续传播事件
    }
    void OnMouseLeave(wxMouseEvent& event)
    {
        wxToolTip::Enable(true);
        if (focosId) {
            focosId = 0;
            Refresh(); // 触发重绘
        }
        event.Skip(); // 继续传播事件
    }
    void onClick(wxMouseEvent& event)
    {
        focosId = GetMousePosIndex(event);
        if (focosId > 0) {
            wxCommandEvent evt(wxEVT_BUTTON);
            evt.SetInt(810);
            wxPoint pt = GetPointFromCirCle(event.GetPosition(), focosId);
            if (focosId == 1)
                evt.SetString("G28"); // 回原点
            else
                evt.SetString(wxString::Format("G91\\r\\nG1 X%d Y%d F6000\\r\\nG90", pt.x, pt.y)); 
            GetParent()->ProcessWindowEvent(evt);
            Refresh(); // 触发重绘
        }
        leftdown_w = 0;        
    }
    void onLeftDown(wxMouseEvent& event)
    {
        focosId = GetMousePosIndex(event);
        if (focosId)
            leftdown_w = 2;
        Refresh(); // 触发重绘
        
    }
     /*#define SQUARE1 16
     #define SQUARE2 42
     #define SQUARE3 77*/
    void onPaint(wxPaintEvent& event)
    {
        wxPaintDC dc(this);
        //dc.Clear();
        dc.SetBackground(*wxWHITE_BRUSH);
        dc.SetPen(wxPen("#EEEEEE"));
        dc.SetBrush(wxBrush("#EEEEEE"));
        dc.DrawCircle(posx, posy, radius3);
        dc.SetBrush(wxBrush("#CECECE"));
        dc.DrawCircle(posx, posy, radius2);
        // dc.SetBrush(*wxWHITE_BRUSH);
        dc.SetPen(wxPen(*wxWHITE, 6, wxPENSTYLE_SOLID));
        dc.DrawLine(posx - radius3, posy - radius3, posx + radius3, posy + radius3);
        dc.DrawLine(posx - radius3, posy + radius3, posx + radius3, posy - radius3);
        dc.SetPen(wxPen("#EEEEEE"));
        dc.SetBrush(wxBrush("#EEEEEE"));
        dc.DrawCircle(posx, posy, radius1);

        dc.SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        dc.SetTextForeground(wxColour("#A0A0A0"));
        dc.DrawText("-X", posx - 86, posy - 6);
        dc.DrawText("X", posx + 78, posy - 6);
        dc.DrawText("Y", posx - 3, posy - 86);
        dc.DrawText("-Y", posx - 9, posy + 78);
        if (bitmap.bmp().IsOk()) {
            dc.DrawBitmap(bitmap.bmp(), wxPoint(posx - IconoffsetX, posy - IconoffsetX));
            //dc.DrawBitmap(bitmap.bmp(), wxPoint(posx - IconSize / 2, posy - IconSize / 2));
        }
        wxColor penColor = wxColour(0, 120, 215);
        // penColor.Set(0, 255 - ((DrawLineWidth -1) * 32), 0, 32);
        dc.SetPen(wxPen(penColor, 3 + leftdown_w, wxPENSTYLE_SOLID));
        //dc.SetPen(wxPen(penColor, 3, wxPENSTYLE_SOLID));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        if (IsBusy)
            return;
        switch (focosId) {
        case 0: break;
        case 1: dc.DrawCircle(posx, posy, radius1+1); break;
        case 2: DrawArcLine(&dc,-SQUARE2, -SQUARE2, -SQUARE2, SQUARE2, -SQUARE1, -SQUARE1, -SQUARE1, SQUARE1); break;
        case 3: DrawArcLine(&dc, SQUARE2, -SQUARE2, -SQUARE2, -SQUARE2, SQUARE1, -SQUARE1, -SQUARE1, -SQUARE1); break;
        case 4: DrawArcLine(&dc, SQUARE2, SQUARE2, SQUARE2, -SQUARE2, SQUARE1, SQUARE1, SQUARE1, -SQUARE1); break;
        case 5: DrawArcLine(&dc, -SQUARE2, SQUARE2, SQUARE2, SQUARE2, -SQUARE1, SQUARE1, SQUARE1, SQUARE1); break;
        case 6: DrawArcLine(&dc, -SQUARE3, -SQUARE3, -SQUARE3, SQUARE3, -SQUARE2, -SQUARE2, -SQUARE2, SQUARE2); break;
        case 7: DrawArcLine(&dc, SQUARE3, -SQUARE3, -SQUARE3, -SQUARE3, SQUARE2, -SQUARE2, -SQUARE2, -SQUARE2); break;
        case 8: DrawArcLine(&dc, SQUARE3, SQUARE3, SQUARE3, -SQUARE3, SQUARE2, SQUARE2, SQUARE2, -SQUARE2); break;
        case 9: DrawArcLine(&dc, -SQUARE3, SQUARE3, SQUARE3, SQUARE3, -SQUARE2, SQUARE2, SQUARE2, SQUARE2); break;
        }
    }
};

class progressPanel : public wxPanel
{
public:
    progressPanel(wxWindow* parent, wxStandardID id = wxID_ANY, wxPoint pos = wxDefaultPosition, wxSize size = wxSize(332, 142))
        : wxPanel(parent, id, pos,size)
    {
        //Bind(wxEVT_PAINT, &progressPanel::onPaint, this);
        this->SetBackgroundColour(wxColour("#F0F0F0"));
        wxStaticText*   title  = new wxStaticText(this, wxID_ANY, "Print Progress");
        imgFree   = ScalableBitmap(this, "OrcaSlicer_192px_transparent", 96);
        //wxBitmap        bitmap(wxString::FromUTF8("E:\\79\\93\\多头test_PLA_5h13m.png"), wxBITMAP_TYPE_PNG);
        if (!imgFree.bmp().IsOk())
            cout << "Failed to load bitmap!" << endl;
        else
            fileImage   = new wxStaticBitmap(this, wxID_ANY, imgFree.bmp(), wxDefaultPosition, wxSize(96, 96));
        fileName    = new wxStaticText(this, wxID_ANY, "Filename");
        statusText  = new wxStaticText(this, wxID_ANY, "Status: Printing");
        progressBar = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition,wxDefaultSize);
        progressBar->SetValue(0); // 设置进度为0%
        wxBoxSizer* allSizer = new wxBoxSizer(wxVERTICAL);
        wxBoxSizer* rightSizer = new wxBoxSizer(wxVERTICAL);
        rightSizer->Add(fileName, 0, wxEXPAND | wxTOP | wxLEFT |wxRIGHT, 10);
        rightSizer->Add(statusText, 0, wxEXPAND | wxTOP | wxLEFT |wxRIGHT, 10);
        rightSizer->Add(progressBar, 0, wxEXPAND | wxTOP | wxLEFT |wxRIGHT, 10);
        wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);
        mainSizer->Add(fileImage, 0, wxEXPAND | wxLEFT |wxBOTTOM, 10);
        mainSizer->Add(rightSizer, 1, wxEXPAND);
        allSizer->Add(title, 0, wxEXPAND | wxLEFT | wxTOP, 10);
        allSizer->Add(mainSizer, 0, wxEXPAND);
        SetSizer(allSizer);
    }
    void SetProgress(int value) { progressBar->SetValue(value); }
    void SetFileName(const wxString& name, const wxString& imagepath = "")
    {
        fileName->SetLabel(name);
        if (imagepath == "")
            fileImage->SetBitmap(imgFree.bmp());
        else {
            wxImage img = wxImage(imagepath);
            fileImage->SetBitmap(img);
        }
        Refresh();
    }
    void SetStatusText(const wxString& status)
    {
        statusText->SetLabel(wxString::Format("Status:%s %d%%", status, progressBar->GetValue()));
    }

private:
    ScalableBitmap  imgFree;
    wxStaticBitmap* fileImage;
    wxStaticText*   fileName;
    wxStaticText*   statusText;
    wxGauge*        progressBar;
    void onPaint(wxPaintEvent& event)
    {
        wxPaintDC dc(this);
        //dc.Clear();
        dc.SetBrush(wxBrush("#F0F0F0"));
        dc.SetPen(wxPen("#C0C0C0"));
        dc.DrawRectangle(wxRect(0, 0, 332, 142));
        event.Skip();           // 继续传播
    }
};

class wmStatusPanel : public wxPanel
{
//#define SQUARE1 16
//#define SQUARE2 42
//#define SQUARE3 77
#define Lbuttonw 148
#define Lbuttonh 48
#define Lbuttonx 3
#define Lbuttony 2//42
#define Lbuttonn 6
#define tempADC_num 5

public://wxSize(588, 480)
    wmStatusPanel(wxWindow* parent, wxStandardID id = wxID_ANY, wxPoint pos = wxDefaultPosition) : wxPanel(parent, id, pos, wxDefaultSize)
    {
        #if 1
        wxBoxSizer* mainSizer  = new wxBoxSizer(wxVERTICAL);        
        wxBoxSizer* midSizer = new wxBoxSizer(wxVERTICAL);
        wxBoxSizer* rightSizer   = new wxBoxSizer(wxVERTICAL);
        wxBoxSizer* topSizer   = new wxBoxSizer(wxHORIZONTAL);
        leftSizer              = new wxBoxSizer(wxVERTICAL);
        wxDisplay mdisplay(wxDisplay::GetFromWindow(this));
        double    scaleX = mdisplay.GetScaleFactor();
        cout << "scaleX:" << scaleX << endl;
        //wxPanel* lefPanel = new wxPanel(this, wxID_ANY);
        for (int i = 0; i < 5; i++) {
            tempButton[i] = new ExtruderPanel(this, i, i < 4, wxDefaultPosition, wxDefaultSize, tempname[i]);
            tempButton[i]->SetMinSize(wxSize(-1, 48 * scaleX));
            leftSizer->Add(tempButton[i], 0, wxEXPAND);
        }
        //lefPanel->SetMinSize(wxSize(-1, 48 * scaleX * 5 * scaleX));
        //lefPanel->SetSizer(leftSizer);
        //tempButton[4] = new ExtruderPanel(this, 4, false, wxDefaultPosition, wxDefaultSize, tempname[4]);
        //leftSizer->Add(tempButton[4], 1, wxEXPAND);
        
        moveBarPanel     = new MoveBarPanel(this, "monitor_axis_home", wxID_ANY, wxDefaultPosition, wxDefaultSize, scaleX);
        
        moveBarPanel->SetMinSize(wxSize(224 * scaleX, 224 * scaleX));
        

        //moveBarPanel->SetMinSize(wxSize(224,224));  //216,216

        m_prog    = new progressPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
        XYZpos[0] = new wxStaticText(this, wxID_ANY, "X=  0.0  ");
        XYZpos[1] = new wxStaticText(this, wxID_ANY, "Y=  0.0  ");
        XYZpos[2] = new wxStaticText(this, wxID_ANY, "Z=  0.0  ");

        wxBoxSizer* xyzSizer = new wxBoxSizer(wxHORIZONTAL);
        xyzSizer->Add(XYZpos[0], 0, wxEXPAND | wxLEFT |wxRIGHT, 5);
        xyzSizer->AddSpacer(5);
        xyzSizer->Add(XYZpos[1], 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
        xyzSizer->AddSpacer(5);
        xyzSizer->Add(XYZpos[2], 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
        midSizer->Add(xyzSizer, 0, wxALIGN_CENTER_HORIZONTAL);
        midSizer->Add(moveBarPanel, 0, wxEXPAND);

        wxBoxSizer* moveBedSizer = new wxBoxSizer(wxHORIZONTAL);
        wxSize      bedMoveSize  = wxSize(48 * scaleX, 32 * scaleX);
        BedMove[0]               = new ItemPanel(this, wxDefaultPosition, bedMoveSize, "10", "", "monitor_bed_up", 18, 1);
        BedMove[1]               = new ItemPanel(this, wxDefaultPosition, bedMoveSize, "1", "", "monitor_bed_up", 18, 1);
        BedMove[2]               = new ItemPanel(this, wxDefaultPosition, bedMoveSize, "1", "", "monitor_bed_down", 18, 1);
        BedMove[3]               = new ItemPanel(this, wxDefaultPosition, bedMoveSize, "10", "", "monitor_bed_down", 18, 1);
        //BedMove[0]->SetMinSize(wxSize(32, 24));
        //BedMove[1]->SetMinSize(wxSize(32, 24));
        //BedMove[2]->SetMinSize(wxSize(32, 24));
        //BedMove[3]->SetMinSize(wxSize(32, 24));

        moveBedSizer->Add(BedMove[0], 0, wxEXPAND);
        moveBedSizer->Add(BedMove[1], 0, wxEXPAND);
        moveBedSizer->AddSpacer(8);
        moveBedSizer->Add(BedMove[2], 0, wxEXPAND);
        moveBedSizer->Add(BedMove[3], 0, wxEXPAND);
        midSizer->Add(moveBedSizer, 0,  wxALIGN_CENTER_HORIZONTAL);

        topSizer->Add(leftSizer, 0, wxEXPAND);
        topSizer->Add(midSizer, 0, wxEXPAND);
        
        meterPanel = new MeterPanel(this, wxID_ANY, wxDefaultPosition, wxSize(64 * scaleX, Lbuttonh * scaleX));
        lampPanel = new FanPanel(this, wxDefaultPosition, wxSize(64 * scaleX, Lbuttonh * scaleX),
                                 "", "monitor_lamp_on", "monitor_lamp_off", 32, false, 1, "Lamp");
        //meterPanel->Hide();
        //lampPanel->Hide();
        wxBoxSizer* lampBoxSizer = new wxBoxSizer(wxHORIZONTAL);
        rightSizer->Add(meterPanel, 0, wxEXPAND);
        rightSizer->Add(lampPanel, 0, wxEXPAND);

        wxBoxSizer* fanBoxSizer = new wxBoxSizer(wxHORIZONTAL);
        for (int i = 0; i < 4; i++) {
            Currentbtn[i] = new wxButton(this, wxID_ANY, wxString::Format("T%d", i), wxDefaultPosition, wxSize(24 * scaleX, 24 * scaleX));
            fanButton[i]  = new FanPanel(this, wxDefaultPosition, wxSize(32 * scaleX, Lbuttonh * scaleX), "", "monitor_fan_on",
                                         "monitor_fan_off", 24, true, 0, wxString(print_fan_s[i]));
            if (i>1)
                fanBoxSizer->Add(fanButton[i],0,wxEXPAND);
            else
                lampBoxSizer->Add(fanButton[i], 0, wxEXPAND);
            Currentbtn[i]->Hide();
        }
        rightSizer->Add(lampBoxSizer, 0, wxEXPAND);
        rightSizer->Add(fanBoxSizer, 0, wxEXPAND);
        topSizer->Add(rightSizer, 0, wxEXPAND );
        mainSizer->Add(topSizer, 0, wxEXPAND);
        mainSizer->Add(m_prog, 0, wxEXPAND | wxTOP,10);

        SetSizer(mainSizer);

        #else
        moveBarPanel = new MoveBarPanel(this, "monitor_axis_home", wxID_ANY, wxPoint(176, Lbuttony + 20), wxSize(216, 216));
        m_prog = new progressPanel(this, wxID_ANY, wxPoint(Lbuttonw, 260));
        for (int i = 0; i < 4; i++) {
            tempButton[i] = new ExtruderPanel(this, i, true, wxPoint(0, Lbuttony + i * Lbuttonh), wxSize(Lbuttonw, Lbuttonh), tempname[i]);
            Currentbtn[i] = new wxButton(this, wxID_ANY, wxString::Format("T%d", i), wxPoint(Lbuttonw, Lbuttony + i * Lbuttonh + 12),
                                         wxSize(24, 24));
            Currentbtn[i]->SetBackgroundColour(wxColour("#FFFFFF"));
            Currentbtn[i]->Bind(wxEVT_BUTTON, [this, i](wxCommandEvent& evt) {
                //wxCommandEvent evt(wxEVT_BUTTON);
                evt.SetInt(810);
                evt.SetString(wxString::Format("T%d", i));
                GetParent()->ProcessWindowEvent(evt);
                //cout << "Switch to extruder T" << i << endl;
            });
            Currentbtn[i]->Hide();
            if (i > 0) {
                tempButton[i]->Hide();                
            }
        }
        tempButton[4] = new ExtruderPanel(this, 4, false, wxPoint(0, Lbuttony + extruderNum * Lbuttonh), wxSize(Lbuttonw, Lbuttonh),
                                          tempname[4]);
        meterPanel    = new MeterPanel(this, wxID_ANY, wxPoint(0, Lbuttony + (extruderNum + 2) * Lbuttonh), wxSize(Lbuttonw / 2, Lbuttonh));
        
        lampPanel = new FanPanel(this, wxPoint(Lbuttonw / 2, Lbuttony + (extruderNum + 2) * Lbuttonh), wxSize((Lbuttonw + 1) / 2, Lbuttonh),
                                 "", "monitor_lamp_on", "monitor_lamp_off", 32, false, 1, "Lamp");
        
        for (int i = 0; i < 4; i++) {
            fanButton[i] = new FanPanel(this, wxPoint((Lbuttonw / 4) * i, Lbuttony + (extruderNum + 1) * Lbuttonh),
                                        wxSize(Lbuttonw / 4, Lbuttonh), "", "monitor_fan_on", "monitor_fan_off", 24, true, 0,
                                        wxString(print_fan_s[i]));
        }
        fanButton[3]->SetDlgMode(false);        
        
        XYZpos[0] = new wxStaticText(this, wxID_ANY, "X=0.0", wxPoint(200, Lbuttony), wxSize(68, 20));
        XYZpos[1] = new wxStaticText(this, wxID_ANY, "Y=0.0", wxPoint(270, Lbuttony), wxSize(68, 20));
        XYZpos[2] = new wxStaticText(this, wxID_ANY, "Z=0.0", wxPoint(340, Lbuttony), wxSize(68, 20));
        #endif
        m_timer   = new wxTimer(this);
        //this->Bind(wxEVT_TIMER, &wmStatusPanel::OnTimer, this);
        
        //
        #if 0
        BedMove[0] = new ItemPanel(this, wxPoint(430, Lbuttony +20), wxSize(48, Lbuttonh), "10", "", "monitor_bed_up", 18, 1);
        BedMove[1] = new ItemPanel(this, wxPoint(430, Lbuttony + 20 + Lbuttonh), wxSize(48, Lbuttonh), "1", "", "monitor_bed_up", 18, 1);
        BedMove[2] = new ItemPanel(this, wxPoint(430, Lbuttony+30 + Lbuttonh * 2), wxSize(48, Lbuttonh), "1", "", "monitor_bed_down", 18, 1);
        BedMove[3] = new ItemPanel(this, wxPoint(430, Lbuttony+30 + Lbuttonh * 3), wxSize(48, Lbuttonh), "10", "", "monitor_bed_down", 18,1);
        #endif

        BedMove[0]->SetBackColor(wxColour("#EEEEEE"));
        BedMove[1]->SetBackColor(wxColour("#CECECE"));
        BedMove[2]->SetBackColor(wxColour("#CECECE"));
        BedMove[3]->SetBackColor(wxColour("#EEEEEE"));
        BedMove[0]->Bind(wxEVT_LEFT_UP, [this](wxMouseEvent&) {
            wxCommandEvent evt(wxEVT_BUTTON);
            evt.SetInt(810);
            evt.SetString("G91\\r\\nG1 Z-10 F300\\r\\nG90");
            GetParent()->ProcessWindowEvent(evt);
         });
        BedMove[1]->Bind(wxEVT_LEFT_UP, [this](wxMouseEvent&) {
            wxCommandEvent evt(wxEVT_BUTTON);
            evt.SetInt(810);
            evt.SetString("G91\\r\\nG1 Z-1 F300\\r\\nG90");
            GetParent()->ProcessWindowEvent(evt);
        });
        BedMove[2]->Bind(wxEVT_LEFT_UP, [this](wxMouseEvent&) {
            wxCommandEvent evt(wxEVT_BUTTON);
            evt.SetInt(810);
            evt.SetString("G91\\r\\nG1 Z1 F300\\r\\nG90");
            GetParent()->ProcessWindowEvent(evt);
        });
        BedMove[3]->Bind(wxEVT_LEFT_UP, [this](wxMouseEvent&) {
            wxCommandEvent evt(wxEVT_BUTTON);
            evt.SetInt(810);
            evt.SetString("G91\\r\\nG1 Z10 F300\\r\\nG90");
            GetParent()->ProcessWindowEvent(evt);
        });
       
    }
    // 标记当前使用的挤出机
    void SetCurrentbtn(int num) { 
        for (int i = 0; i < 4; i++) {
            Currentbtn[i]->Show();
            if (num == i)
                Currentbtn[i]->SetBackgroundColour(wxColour("#808080"));
            else
                Currentbtn[i]->SetBackgroundColour(wxColour("#FFFFFF"));
        }
    }

    void UpdaePrintInfo(json sjson)
    {
        int erro_i = 0;
        try {
            if (sjson.contains("error")) {
                // if (sjson["error"][0]["code"].get<int>() == 400) {
                //
                erroMessage = wxString::Format("code:%d,message:%s", sjson["error"]["code"].get<int>(),
                                               wxString::FromUTF8(sjson["error"]["message"].get<string>()));
                m_timer->StartOnce(2000);
                Refresh();
                return;
            }
            for (int i = 0; i < 5; i++) {
                if (sjson.contains(tempname[i])) {
                    if (sjson[tempname[i]].contains("temperature")) {
                        tempButton[i]->SetTemp(sjson[tempname[i]]["temperature"].get<double>());
                        // temperature[i * 2] = sjson[tempname[i]]["temperature"].get<double>();
                    }
                    if (sjson[tempname[i]].contains("target")) {
                        // temperature[i * 2 + 1] = sjson[tempname[i]]["target"].get<double>();
                        tempButton[i]->UpdateTarget(sjson[tempname[i]]["target"].get<double>());
                    }
                    if (sjson[tempname[i]].contains("power")) {
                        tempButton[i]->SetActive(sjson[tempname[i]]["power"].get<double>());
                    }
                }
            }
            erro_i++;
            for (int i = 0; i < 8; i++) {
                if (sjson.contains(print_fan_s[i])) {
                    if (sjson[print_fan_s[i]].contains("rpm") && !sjson[print_fan_s[i]]["rpm"].is_null()) {
                        fanButton[i]->SetLampState((int)sjson[print_fan_s[i]]["rpm"].get<float>());
                    }
                    else if (sjson[print_fan_s[i]].contains("speed")) {
                        fanSpeed[i] = int((double) sjson[print_fan_s[i]]["speed"].get<float>() * 100);
                        if (i > 3) {
                            tempButton[i - 4]->SetTempFan(fanSpeed[i] > 0);
                        } else
                            fanButton[i]->SetLampState(fanSpeed[i]);
                    }
                }
            }
            if (sjson.contains("heater_fan hotend_fan")) {
                if (sjson["heater_fan hotend_fan"].contains("speed")) {
                    //cout << "hotend_fan speed:" << sjson["heater_fan hotend_fan"]["speed"].get<float>() << endl;
                    fanSpeed[0] = int((double) sjson["heater_fan hotend_fan"]["speed"].get<float>() * 100);
                    tempButton[0]->SetTempFan(fanSpeed[0]>0);
                }
            }
            erro_i++;
            for (int i = 0; i < 4; i++) {
                if (sjson.contains(filament_T[i]) && sjson[filament_T[i]].contains("color_data")) {
                    filament_C[i] = wxColour(sjson[filament_T[i]]["color_data"][0][0].get<float>() * 255,
                                             sjson[filament_T[i]]["color_data"][0][1].get<float>() * 255,
                                             sjson[filament_T[i]]["color_data"][0][2].get<float>() * 255);                    
                    tempButton[i]->SetFilamentColor(filament_C[i]);
                }
            }
            erro_i++;
            if (sjson.contains("print_stats")) {
                if (sjson["print_stats"].contains("filename")) {                    
                    printerName = wxString::FromUTF8(sjson["print_stats"].at("filename"));
                    m_prog->SetFileName(printerName);
                }
                if (sjson["print_stats"].contains("state")) {
                    printerStatus = wxString::FromUTF8(sjson["print_stats"].at("state"));
                    m_prog->SetStatusText(printerStatus);
                    SetBusy(printerStatus == "printing");
                }
            }
            erro_i++;
            if (sjson.contains("toolhead")) {
                if (sjson["toolhead"].contains("homed_axes")) {
                    std::string tmp = sjson["toolhead"]["homed_axes"].get<string>();
                    cout << "homed_axes:" << tmp << endl;
                    if (tmp.find("z") != std::string::npos)
                        Z_homed = true; // Z ok
                    else
                        Z_homed = false; // z no
                    if (tmp.find("xy") != std::string::npos)
                        moveBarPanel->SetXHomed(true); // X_homed      = true;  // x ok
                    else
                        moveBarPanel->SetXHomed(false); // X_homed = false; // x no
                }
                if (sjson["toolhead"].contains("extruder")) {
                    string ext_name = sjson["toolhead"]["extruder"];
                    for (int i = 0; i < 4; i++)
                    {
                        if (ext_name == tempname[i]) {
                            SetCurrentbtn(i);
                            break;
                        } 
                    }
                    cout << "current extruder:" << ext_name << endl;
                }
                for (int i = 0; i < 3; i++) {
                    headPos[i] = sjson["toolhead"]["position"][i].get<double>();
                    XYZpos[i]->SetLabel(wxString::Format("%c=%.2f", 'X' + i, headPos[i]));
                }
                std::string::npos;
                // std::cout << "X:" << headPos[0] << " Y:" << headPos[1] << " Z:" << headPos[2] << endl;
            }
            erro_i++;
            if (sjson.contains("display_status")) {
                if (sjson["display_status"].contains("progress")) {
                    if (sjson["display_status"]["progress"].is_null()) {
                        printerprogress = "";
                        m_prog->SetProgress(0);
                    }else {
                        printerprogress = wxString::Format("%.1f%%", sjson["display_status"]["progress"].get<float>());
                        m_prog->SetProgress(100 * sjson["display_status"]["progress"].get<float>());
                    }
                }
                
            }
            erro_i++;
            //Refresh();
        } catch (const json::parse_error& e) {
            // 捕获 JSON 解析错误
            std::cout << "JSON parse error: " << e.what() << std::endl;
            std::cout << erro_i << "Byte position of error: " << e.byte << std::endl;
        } catch (const json::type_error& e) {
            // 捕获类型转换错误（例如，尝试将字符串转换为数字）
            std::cout << erro_i << "JSON type error: " << e.what() << std::endl;
        } catch (const std::exception& e) {
            // 捕获其他所有标准异常
            std::cout << erro_i << "Unexpected error: " << e.what() << std::endl;
        }
    }
    
    void SetHostIp(wxString iphost)
    {
        hostIp = iphost;
        extruderNum = 1;
        if (hostIp == "") {            
            printerName     = "";
            printerStatus   = "Not connected";
            printerprogress = "";
            // memset(temperature, 0, sizeof(temperature));
            memset(headPos, 0, sizeof(headPos));
            Refresh();
        } else {
            wxString url = wxString::Format("http://%s/printer/objects/list", iphost);
            wxString result;
            HttpJsonClient::SendGetRequest(url, result);
            if (result.Contains("\"extruder1\"")) {
                extruderNum++;
            }
            if (result.Contains("\"extruder2\"")) {
                extruderNum++;
            }
            if (result.Contains("\"extruder3\"")) {
                extruderNum++;
            }
            
            SetExtruderNum();
            cout << "extruderNum:" << extruderNum << endl;
        }
    }
    
    void SetBusy(bool b)
    { 
        //IsBusy = b;
        moveBarPanel->SetBusy(b);
        for (int i = 0; i < 4; i++) {
            Currentbtn[i]->Enable(!b);
            BedMove[i]->Enable(!b);
            tempButton[i]->Enable(!b);
            fanButton[i]->Enable(!b);
        }
        tempButton[4]->Enable(!b);
    }

    void SetExtruderNum()
    {   
        #if 1
        leftSizer->Clear();
        leftSizer->Add(tempButton[0], 0, wxEXPAND);
        if (extruderNum != 1) {
            tempButton[1]->Show();
            tempButton[2]->Show();
            tempButton[3]->Show();
            leftSizer->Add(tempButton[1], 0, wxEXPAND);
            leftSizer->Add(tempButton[2], 0, wxEXPAND);
            leftSizer->Add(tempButton[3], 0, wxEXPAND);
        } else {
            tempButton[1]->Hide();
            tempButton[2]->Hide();
            tempButton[3]->Hide();
        }
        leftSizer->Add(tempButton[4], 0, wxEXPAND);
        this->Layout();
        #else
        if (extruderNum != 1) {
            //tempButton[0]->UpdateType(1);
            for (int i = 0; i < 4; i++) {
                tempButton[i]->Show();
                Currentbtn[i]->Show();
            }
        } else {
            //tempButton[0]->UpdateType(0);
            for (int i = 1; i < 4; i++) {
                tempButton[i]->Hide();
                Currentbtn[i]->Hide();
            }
            //tempButton[0]->SetFilamentColor(*wxWHITE);
            Currentbtn[0]->Hide();
        }
        tempButton[4]->SetPosition(wxPoint(0, Lbuttony + extruderNum * Lbuttonh));
        meterPanel->SetPosition(wxPoint(0, Lbuttony + (extruderNum + 2) * Lbuttonh));
        lampPanel->SetPosition(wxPoint(Lbuttonw / 2, Lbuttony + (extruderNum + 2) * Lbuttonh));
        for (int i = 0; i < 4; i++)
            fanButton[i]->SetPosition(wxPoint((Lbuttonw / 4) * i, Lbuttony + (extruderNum + 1) * Lbuttonh));
        #endif
    }
 
    int  GeFilamentColor(wxColour* m_color)
    {
        int num = 0;
        if (hostIp != "") {
            num = 4;
            memcpy(m_color, filament_C, sizeof(filament_C));
        }
        return num;
    }
    void SetCpuUsage(float usage) { meterPanel->SetCpuUsage(usage); }
private:
    progressPanel* m_prog;
    ExtruderPanel* tempButton[5];
    bool           Extruderstatus[5] = {1,0,0,0,1};
    wxTimer* m_timer;
    wxBoxSizer*    leftSizer;
    MoveBarPanel* moveBarPanel;

    wxString hostIp   = "";
    wxString erroMessage    = "";
    wxString printerName    = "";
    wxString printerStatus  = "Not connected";
    wxString printerprogress= "";
    bool     Z_homed        = false;
    double   headPos[4]     = {0};
    const char*    tempname[tempADC_num]        = {"extruder", "extruder1", "extruder2", "extruder3", "heater_bed"};
    const char*    print_fan_s[8]               = {"fan",
                                                   "fan_generic auxiliary_fan",
                                                   "fan_generic air_fan",
                                                   "controller_fan motherboard_fan",
                                                   "heater_fan hotend0_fan",
                                                   "heater_fan hotend1_fan",
                                                   "heater_fan hotend2_fan",
                                                   "heater_fan hotend3_fan"};
    const char*    filament_T[4] = {"neopixel T0_RGB", "neopixel T1_RGB", "neopixel T2_RGB", "neopixel T3_RGB"};
    wxColour       filament_C[4];
    int      fanSpeed[8]    = {0};
    int      DrawLineWidth  = 1;
    MeterPanel*     meterPanel;
    wxStaticText*   XYZpos[4];
    FanPanel*       lampPanel;
    FanPanel*       fanButton[4];
    ItemPanel*      BedMove[4];
    wxButton*       Currentbtn[4];
    int             extruderNum = 1;
    
    void OnTimer(wxTimerEvent& event) { 
        erroMessage = "";
    }
};
class FarmManager : public wxPanel
{
    #if 1
#define PRINTER_PING 8080
#define PRINTER_STATUS 8081
#define PRINTER_START 8082
#define PRINTER_CANEL 8083
#define PRINTER_PAUSE 8084
#define PRINTER_RESUME 8085
#define FARM_ID_BASE 20000
#define PEINTERS_FILE_PATH "\\resources\\info\\"
#define PEINTERS_FILE_NAME "auto_save.ini"
#define PRINT
    enum IDX_COLUMN {
        idx_COL_NO     = 0,
        idx_COL_GROUP  = 1,
        idx_COL_IPADDR = 2,
        idx_COL_STATUS = 3,
        idx_COL_PROG   = 4,
        idx_COL_INFO   = 5,
        idx_Max
    };
   #endif
    //wxString PrintStaus[5]={"timeout", "standby", "printing", "paused","complete" "erro"};
    struct PrinterInfo { 
        int      n;
        int      g;
        wxString Ip;
        int s;
    };

public:
    FarmManager(wxWindow*      parent,
                wxString       fpath,
                wxWindowID     winid = wxID_ANY,
                const wxPoint& pos   = wxDefaultPosition,
                const wxSize&  size  = wxDefaultSize)
        : wxPanel(parent, winid, pos, size), file_path(fpath)
    {
        wxStaticText* m_title = new wxStaticText(this, wxID_ANY, "");
        std::vector<std::string> LocalIpList = HttpJsonClient::get_local_ipv4_addresses();
        if (LocalIpList.size() > 0) {
            wxString ip_addresses;
            for (size_t i = 0; i < LocalIpList.size(); i++) {
                ip_addresses += LocalIpList[i];
                if (i != LocalIpList.size() - 1)
                    ip_addresses += ", ";
            }
            m_title->SetLabelText(wxString::Format("Farm Manager(%s)", ip_addresses));
        } else {
            LocalIpList.push_back("");
            m_title->SetLabelText("Farm Manager(No IP addresses detected)");
        }
        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
        mainSizer->Add(m_title,0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
        
        //LocalIpList[0]      = "172.16.20.23";
        wxButton* m_button1 = new wxButton(this, wxID_ANY, "Save");
        wxButton*  m_button2 = new wxButton(this, wxID_ANY, "Load");
        wxButton*  m_button  = new wxButton(this, wxID_ANY, "Searh");
        wxButton*  m_button3 = new wxButton(this, wxID_ANY, "Print");
        wxButton*  m_button4 = new wxButton(this, wxID_ANY, "Calib");
        wxButton*  m_button5 = new wxButton(this, wxID_ANY, "Canel");
        wxButton*  m_button6 = new wxButton(this, wxID_ANY, "Pause");
        m_inputctrl = new wxTextCtrl(this, wxID_ANY, LocalIpList[0], wxDefaultPosition, wxDefaultSize, 0, wxTextValidator(wxFILTER_NUMERIC));
        m_button3->Enable(false);
        m_button4->Enable(false);
        m_button5->Enable(false);
        m_button6->Enable(false);
        wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
        buttonSizer->Add(m_button2, 1, wxEXPAND | wxLEFT | wxTOP | wxBOTTOM, 5);
        buttonSizer->Add(m_button1, 1, wxEXPAND | wxLEFT | wxTOP | wxBOTTOM, 5);
        buttonSizer->Add(m_inputctrl, 2, wxEXPAND | wxLEFT | wxTOP | wxBOTTOM, 5);
        buttonSizer->Add(m_button, 1, wxEXPAND | wxLEFT | wxTOP | wxBOTTOM, 5);
        buttonSizer->Add(m_button3, 1, wxEXPAND | wxLEFT | wxTOP | wxBOTTOM, 5);
        buttonSizer->Add(m_button4, 1, wxEXPAND | wxLEFT | wxTOP | wxBOTTOM, 5);
        buttonSizer->Add(m_button5, 1, wxEXPAND | wxLEFT | wxTOP | wxBOTTOM, 5);
        buttonSizer->Add(m_button6, 1, wxEXPAND | wxLEFT | wxTOP | wxBOTTOM, 5);
        mainSizer->Add(buttonSizer);
        m_button->Bind(wxEVT_BUTTON, &FarmManager::OnSearchPrinter, this);
        m_button1->Bind(wxEVT_BUTTON, [this](wxCommandEvent& evt) {
            wxFileDialog openFileDialog(this, _("Save file"), file_path, "", "", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
            if (openFileDialog.ShowModal() == wxID_OK) {
                // 用户选择了文件，获取路径
                wxString filePath = openFileDialog.GetPath();
                SavePrinterListToFile(filePath);
                SavePrinterListToFile(file_path + PEINTERS_FILE_NAME);
            }
        });
        m_button2->Bind(wxEVT_BUTTON, [this](wxCommandEvent& evt) {
            wxFileDialog openFileDialog(this, _("Select file"), file_path, "", "", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
            if (openFileDialog.ShowModal() == wxID_OK) {
                // 用户选择了文件，获取路径
                wxString filePath = openFileDialog.GetPath();
                LoadPrinterListFromFile(filePath);
            }
        });
        m_listCtrl = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES);
        m_listCtrl->SetMinSize(wxSize(800,600));
        wxBoxSizer* listSizer = new wxBoxSizer(wxHORIZONTAL);
        listSizer->Add(m_listCtrl, 1, wxEXPAND | wxALL);
        mainSizer->Add(listSizer, 1, wxEXPAND | wxALL);
       
        /*wxSize size1 = GetClientSize();
        size1        = wxSize(800, 600);
        m_listCtrl   = new wxListCtrl(this, wxID_ANY, wxPoint(5, 70), size1, wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES);*/
        // 设置列（带表头）
        m_listCtrl->InsertColumn(idx_COL_NO, L"No.", wxLIST_FORMAT_LEFT, 40);
        m_listCtrl->InsertColumn(idx_COL_GROUP, L"▼Group", wxLIST_FORMAT_LEFT, 80);
        m_listCtrl->InsertColumn(idx_COL_IPADDR, L"Ip address", wxLIST_FORMAT_LEFT, 120);
        m_listCtrl->InsertColumn(idx_COL_STATUS, L"▼Status", wxLIST_FORMAT_LEFT, 90);
        m_listCtrl->InsertColumn(idx_COL_PROG, L"PROG", wxLIST_FORMAT_LEFT, 60);
        m_listCtrl->InsertColumn(idx_COL_INFO, L"Info", wxLIST_FORMAT_LEFT, 410);
        
        /*for (int i = 0; i < idx_Max;i++)
            m_listCtrl->SetColumnWidth(i, wxLIST_AUTOSIZE);*/
        wxItemAttr headerAttr;
        headerAttr.SetTextColour(*wxBLACK);
        //headerAttr.SetBackgroundColour(wxColour(192, 192, 192));
        headerAttr.SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        m_listCtrl->SetHeaderAttr(headerAttr);
        m_timer = new wxTimer(this, 7001);
        m_timer_edit = new wxTimer(this, 7002);
        //m_timer_edit->Bind(wxEVT_TIMER, &FarmManager::onTimer, this);
        Bind(wxEVT_TIMER, &FarmManager::onTimer, this);
        Bind(wxEVT_COMMAND_TEXT_UPDATED, &FarmManager::OnResultUpdated, this);
        m_listCtrl->Bind(wxEVT_LEFT_DCLICK, &FarmManager::onLeftDClick, this);
        m_listCtrl->Bind(wxEVT_LIST_ITEM_RIGHT_CLICK, &FarmManager::onRightUp, this);
        // m_listCtrl->Bind(wxEVT_RIGHT_UP, &FarmManager::onRightUp, this);
        if (file_path == "") {
            std::wstring path(size_t(MAX_PATH_Len), wchar_t(0));
            int          len = int(::GetModuleFileName(nullptr, path.data(), MAX_PATH_Len));
            if (len > 0 && len < MAX_PATH_Len) {
                path.erase(path.begin() + len, path.end());
            }
            wxFileName mfileName(path);
            file_path = mfileName.GetPath() + PEINTERS_FILE_PATH;
            //file_path = BrowserTabPanel::GetAppPath() + PEINTERS_FILE_PATH;

        } else
            file_path += PEINTERS_FILE_PATH;

        LoadPrinterListFromFile(file_path + PEINTERS_FILE_NAME);
        Bind(wxEVT_COMMAND_MENU_SELECTED, &FarmManager::OnCommandSelect, this);
        Bind(wxEVT_LIST_COL_CLICK, &FarmManager::OnListColClick, this);
        SetSizer(mainSizer);
        //m_listCtrl->SetAutoLayout(true);
        //mainSizer->SetSizeHints(this);
    }
    void SetActive(bool isOn)
    {
        if (IpList.size() > 0) {
            isActive = isOn;
            m_timer->Start(100, true);
        }
    }
    long AddPrinterItem(int index, wxString group, wxString ipaddr)
    {
        long itemIdx = m_listCtrl->InsertItem(m_listCtrl->GetItemCount(), wxString::Format("%d", index));
        m_listCtrl->SetItemTextColour(itemIdx, *wxBLACK);
        m_listCtrl->SetItem(itemIdx, idx_COL_GROUP, group);
        m_listCtrl->SetItem(itemIdx, idx_COL_IPADDR, ipaddr);
        return itemIdx;
    }
    long AddListPrinter(int index, wxString group, wxString ipaddr)
    {
        int idx_g = AddGroup(idx_COL_GROUP, group);
        IpList.push_back(PrinterInfo{index + 1, idx_g, ipaddr, 0});
        //cout << wxString::Format("%d:%s,group_i=%d\n", index + 1, ipaddr, idx_g);
        return AddPrinterItem(index + 1, group, ipaddr);
    }

    void UpdatePrinter(int idx, wxString info)
    {
        wxStringTokenizer tokenizer(info, "|");
        wxString          status = tokenizer.GetNextToken();
        IpList[idx].s            = AddGroup(idx_COL_STATUS, status);
        if (GroupActive[idx_COL_STATUS][IpList[idx].s] && GroupActive[idx_COL_GROUP][IpList[idx].g])
        {//update or re-add
            long id = m_listCtrl->FindItem(-1, wxString::Format("%d", IpList[idx].n ));
            if (id < 0) { // 因为状态曾经被删除，改变后重新加回去。
                id = AddPrinterItem(IpList[idx].n, GroupList[idx_COL_GROUP][IpList[idx].g], IpList[idx].Ip);
                cout << idx << ":Status changed, re-add\n";
            }
            if (id >= 0) {
                if (status == "error" || status == "timeout")
                    m_listCtrl->SetItemTextColour(id, wxColour("#800000"));
                else if (status == "standby")
                    m_listCtrl->SetItemTextColour(id, wxColour("#000080"));
                else
                    m_listCtrl->SetItemTextColour(id, *wxBLACK);
                m_listCtrl->SetItem(id, idx_COL_STATUS, status);
                m_listCtrl->SetItem(id, idx_COL_PROG, tokenizer.GetNextToken());
                wxString info1 = tokenizer.GetNextToken();
                if (status != "standby" || !standbyActive)
                    m_listCtrl->SetItem(id, idx_COL_INFO, info1);
            }
        } else {
            //delete Item
            long id = m_listCtrl->FindItem(-1, wxString::Format("%d", IpList[idx].n));
            if (id >= 0) {
                m_listCtrl->DeleteItem(id);
                cout << idx << ":Status changed, delete\n";
            }
        }
    }

    static void funPing(void* arg, wxEvtHandler* m_parent, int id = 0)
    {
        wxString       url = wxString::Format("http://%s/printer/info", *(wxString*) arg);
        wxCommandEvent evt(wxEVT_COMMAND_TEXT_UPDATED, PRINTER_PING);
        evt.SetString("");
        evt.SetInt(id);
        wxString jsonResult;
        if (HttpJsonClient::GetPrinterJson(url, &jsonResult)) {
            evt.SetString(*(wxString*) arg);
        }
        wxPostEvent(m_parent, evt);
        delete arg;
    }

    static void funQurey(void* arg, wxEvtHandler* m_parent, int id = 0)
    {
        wxString       url = wxString::Format("http://%s/printer/objects/query", *(wxString*) arg);
        wxCommandEvent evt(wxEVT_COMMAND_TEXT_UPDATED, PRINTER_STATUS);
        evt.SetString("timeout||");
        evt.SetInt(id);
        wxString jsonResult;
        wxString postData = "{\"objects\": {\"print_stats\": [\"state\", \"filename\"], \"display_status\": [\"progress\"]}}";
        if (HttpJsonClient::SendPostRequest(url, postData, "application/json", jsonResult)) {
            try {
                json sroot = json::parse(jsonResult);
                if (sroot.contains("result") && sroot["result"].contains("status")) {
                    json sjson = sroot["result"]["status"];
                    if (sjson.contains("print_stats") && sjson["print_stats"].contains("state")) {
                        if (!sjson["print_stats"]["state"].is_null()) {
                            wxString status   = wxString::FromUTF8(sjson["print_stats"]["state"].get<std::string>());
                            wxString info     = "";
                            double   progress = 0;
                            if (sjson["print_stats"].contains("filename")) {
                                info = wxString::FromUTF8(sjson["print_stats"]["filename"].get<std::string>());
                            }
                            if (sjson.contains("display_status") && sjson["display_status"].contains("progress")) {
                                progress = sjson["display_status"]["progress"].get<double>();
                            }
                            evt.SetString(wxString::Format("%s|%.1f%%|%s", status, progress * 100, info));
                        } else
                            evt.SetString("error||");
                    }
                }
            } catch (json::parse_error& e) {
                std::cout << "funQurey parsing error: " << e.what() << std::endl;
            } catch (json::type_error& e) {
                cout << url << endl;
                std::cout << "funQurey error: " << e.what() << std::endl;
            } catch (std::exception& e) {
                std::cout << "funQurey Exception: " << e.what() << std::endl;
            }
        }
        // evt.SetString(jsonResult);
        wxPostEvent(m_parent, evt);
        delete arg;
    }

    static void fun3(void* arg, wxEvtHandler* m_parent, int id = 0)
    {
        wxString       url = *(wxString*) arg;
        wxCommandEvent evt(wxEVT_COMMAND_TEXT_UPDATED, PRINTER_PING);
        evt.SetString("");
        evt.SetInt(id);
        wxString jsonResult;
        if (HttpJsonClient::GetPrinterJson(url, &jsonResult)) {
            evt.SetString(*(wxString*) arg);
        }
        wxPostEvent(m_parent, evt);
        delete arg;
    }

    void SavePrinterListToFile(wxString filename)
    {
        wxTextFile file;
        if (wxFileExists(filename)) {
            file.Open(filename);
            file.Clear();
        } else {
            file.Create(filename);
        }
        int num = m_listCtrl->GetItemCount();
        for (int i = 0; i < num; i++) {
            wxString line = wxString::Format("%s,%s", m_listCtrl->GetItemText(i, idx_COL_GROUP), m_listCtrl->GetItemText(i, idx_COL_IPADDR));
            file.AddLine(line);
        }
        file.Write();
        file.Close();
    }

    void LoadPrinterListFromFile(wxString filename)
    {
        wxTextFile file;
        if (wxFileExists(filename)) {
            m_listCtrl->DeleteAllItems();
            IpList.clear();
            GroupList[idx_COL_GROUP].clear();            
            GroupActive[idx_COL_GROUP].clear();
            /*GroupList[3].clear();
            GroupActive[3].clear();*/
            file.Open(filename);
            int n = 0;
            for (wxString str = file.GetFirstLine(); !file.Eof(); str = file.GetNextLine()) {
                wxStringTokenizer tokenizer(str, ",");
                wxString          group  = tokenizer.GetNextToken();
                wxString          ipaddr = tokenizer.GetNextToken();                
                AddListPrinter(n, group, ipaddr);
                n++;
            }
            file.Close();
            if (m_listCtrl->GetItemCount() > 0) {
                m_timer->Start(100, true);
                // isActive = true;
            }
        } else
            cout << "File not found: " << filename << endl;
    }

private:
#if 1
    wxTextCtrl*   m_inputctrl;
    wxStaticText* m_statictxt;
    wxListCtrl*   m_listCtrl;
    wxTimer*      m_timer;
    wxTimer*      m_timer_edit;
    wxTextCtrl*   m_editor    = nullptr;
    wxComboBox*   m_filecombo = nullptr;
    wxString      file_path;
    wxArrayString         m_filelist;
    long                  m_editRow = -1;
    long                  m_editCol = -1;
    std::vector<PrinterInfo> IpList;
    std::vector<wxString> GroupList[idx_Max];
    std::vector<bool>     GroupActive[idx_Max];
    int                   stepPing = 1;
    int                   stepQuery;
    int                   maxStepFun1 = 255;
    int                   PrintIndex  = 1;
    long                  ip[4];
    bool                  isActive = false;
    bool                  standbyActive = false;
 
#endif
    void onTimer(wxTimerEvent& event)
    {
        if (event.GetId() == 7001) {
            stepQuery = 0;
            for (int i = 0; i < 4; i++) {
                //if (stepQuery < IpList.size())
                CreateThredFunQuery();
            }
            m_timer->Start(10000, true);
        }
        if (event.GetId() == 7002) {
            standbyActive = false;
            cout << "timer_7002\n";
        }
        // cout << "FarmManager onTimer:" << event.GetId() << endl;
    }

    int AddGroup(int col, wxString value)
    {
        auto it = std::find(GroupList[col].begin(), GroupList[col].end(), value);
        int  index  = it - GroupList[col].begin();
        //if (it == GroupList[col].end()) {
        if (index == GroupList[col].size()){
            GroupList[col].push_back(value);
            GroupActive[col].push_back(true);
        }
        return index;
    }
   
    void CreateThredFunPing(){
        if (stepPing < maxStepFun1) {
            wxString*   strip  = new wxString(wxString::Format("%d.%d.%d.%d", ip[0], ip[1], ip[2], stepPing));
            FuncThread* thread = new FuncThread(FarmManager::funPing, strip, this);
            thread->Run();
            stepPing++;
        } else {
            isActive  = true;
            stepQuery = 0;
            m_timer->Start(100, true);
            SavePrinterListToFile(file_path + PEINTERS_FILE_PATH);
        }
    }
    
    void CreateThredFunQuery()
    {        
        if (isActive) {
            //wxString    group  = m_listCtrl->GetItemText(stepQuery, idx_COL_GROUP);
            while (stepQuery < IpList.size()) {
                //int      n     = stepQuery;
                stepQuery++;
                if (GroupActive[idx_COL_GROUP][IpList[stepQuery-1].g]) {
                    //group select on
                    wxString*   strip  = new wxString(IpList[stepQuery-1].Ip);
                    FuncThread* thread = new FuncThread(FarmManager::funQurey, strip, this, stepQuery-1);
                    thread->Run();
                    break;
                }
            }
        }
    }
 
    void UpdateDisplay(int col) {
        for (PrinterInfo p : IpList) {
            //if (GroupActive[idx_COL_STATUS][statusindex])
            long id = m_listCtrl->FindItem(-1, wxString::Format("%d", p.n ));
            if (GroupActive[idx_COL_GROUP][p.g] && GroupActive[idx_COL_STATUS][p.s]) {
                if (id < 0) {
                    id = AddPrinterItem(p.n, GroupList[idx_COL_GROUP][p.g], p.Ip);
                    m_listCtrl->SetItem(id, idx_COL_STATUS, GroupList[idx_COL_STATUS][p.s]);
                    cout << p.n << ":UpdateDisplay, re-add\n";
                }
            } else {//delete item
                if (id >= 0) {
                    m_listCtrl->DeleteItem(id);
                    cout << p.n << ":UpdateDisplay, delete\n";
                }
            }
        }
        m_timer->Start(100, true);
    }
    
    void OnColMenuItemSelect(wxCommandEvent& event)
    {
        int      menuItemId = event.GetId() - FARM_ID_BASE -8003;
        int      column     = menuItemId /100;
        int      idx        = menuItemId % 100;
        //wxString value      = GroupList[column][idx];
        GroupActive[column][idx] = !GroupActive[column][idx];
        //UpdateDisplay(column);
        wxListEvent evt(wxEVT_LIST_COL_CLICK);
        evt.SetColumn(column);
        ProcessWindowEvent(evt); 
        //event.Veto();  
    }
    
    wxMenu* CreateFilterMenu(int column)
    {
        wxMenu* menu = new wxMenu;
        menu->Append(FARM_ID_BASE + 8000 + column * 100, L"Select all");
        // 添加"全不选"选项
        menu->Append(FARM_ID_BASE + 8001 + column * 100, L"全不选");
        menu->AppendSeparator();//加线条
        for (int i = 0; i < GroupList[column].size();i++) {
            int         itemId = FARM_ID_BASE + 8003 + column * 100 + i;

            //wxMenuItem* item   = new wxMenuItem(menu, itemId, GroupList[column][i], "", wxITEM_CHECK);
            wxMenuItem* item = menu->AppendCheckItem(itemId, GroupList[column][i]);
            if (GroupActive[column][i])
                item->Check(true);
            //menu->Append(item);
            Bind(wxEVT_COMMAND_MENU_SELECTED, &FarmManager::OnColMenuItemSelect, this, itemId);
        }
        menu->AppendSeparator();
        menu->Append(FARM_ID_BASE + 8002 + column * 100, L"OK");
        Bind(wxEVT_MENU, [this, column](wxCommandEvent& event) { UpdateDisplay(column); }, FARM_ID_BASE + 8002 + column * 100);
        Bind(
            wxEVT_MENU,
            [this, column](wxCommandEvent& event) {
                for (int i = 0; i < GroupActive[column].size(); i++)
                    GroupActive[column][i] = true;
                UpdateDisplay(column);
            },FARM_ID_BASE + 8000 + column * 100);
        Bind(
            wxEVT_MENU,
            [this, column](wxCommandEvent& event) {
                for (int i = 0; i < GroupActive[column].size(); i++)
                    GroupActive[column][i] = false;
                UpdateDisplay(column);
            },FARM_ID_BASE + 8001 + column * 100);
        return menu;
    }

    void OnListColClick(wxListEvent& evt) { 
        m_editCol = evt.GetColumn();
        if (m_editCol == 3 || m_editCol == 1) {
            // GetGroup(m_editCol);
            wxPoint pt = wxPoint(45, 80);
            if (m_editCol == 3)
                pt.x += 200;
            wxMenu* menu = CreateFilterMenu(m_editCol);
            PopupMenu(menu, pt);
            delete menu;
        }
        evt.Skip();
    }
    
    void OnEditComplete(wxEvent& evt)
    {
        if (m_editor && m_editRow != -1 && m_editCol != -1) {
            // 将编辑器内容更新到列表中
            wxString strInput = m_editor->GetValue();
            cout << "Edit complete at row: " << m_editRow << ", col: " << m_editCol << ", value: " << strInput << endl;            
            m_listCtrl->SetItem(m_editRow, m_editCol, strInput);
            if (m_editRow == idx_COL_GROUP) {
                wxString strN = m_listCtrl->GetItemText(m_editRow,0);
                int g = AddGroup(idx_COL_GROUP, strInput);
                for (auto it:IpList){
                    if (it.n == strN) {
                        it.g = g;
                        break;
                    }
                }                
            }
        }
        m_editor->Hide();
    }
    
    bool ReadFileList(const wxString& m_ip, wxArrayString* filelist)
    {
        wxString       url = wxString::Format(m_File_url, m_ip);
        wxString       response;
        wxCommandEvent evt(wxEVT_COMMAND_TEXT_UPDATED, IMAGE_DOWNLOAD_FINISH);
        evt.SetString("filename");
        int idx = 0;
        filelist->Add("");
        if (HttpJsonClient::SendGetRequest(url, response)) {
            try {
                json  sjson = json::parse(response);
                json files = sjson["result"]["files"];
                if (!files.is_null()) {
                    for (auto& file : files) {
                        wxString wx_name = wxString::FromUTF8(file[JosnfileName].get<string>());
                        filelist->Add(wx_name);
                    }
                    return true;
                }
            }
            catch (json::parse_error& e) {
                std::cout << "JSON parsing error: " << e.what() << std::endl;
            } catch (json::type_error& e) {
                std::cout << "JSON type error: " << e.what() << std::endl; }
            catch (std::exception& e) {
                std::cout << "Exception: " << e.what() << std::endl;
            }
        }
        return false;
    }
    
    void SelectAllFileFromPrint(wxRect rect)
    {
        if (m_filecombo) {
            m_filecombo->Destroy();
        }
        standbyActive = true;
        m_timer_edit->Start(30000, true);
        m_filelist.clear();
        ReadFileList(m_listCtrl->GetItemText(m_editRow, 2), &m_filelist);
        m_filecombo = new wxComboBox(m_listCtrl, wxID_ANY, "", rect.GetTopLeft(), rect.GetSize(), m_filelist, wxCB_READONLY);
        // m_filecombo->SetForegroundColour("#808080");
        //m_filecombo->SetHint("Select file from priter");
        m_filecombo->SetFocus();
        m_filecombo->Popup();
        m_filecombo->Bind(wxEVT_COMBOBOX, [this](wxCommandEvent& evt) {
            m_listCtrl->SetItem(m_editRow, idx_COL_INFO, m_filecombo->GetValue());
            m_filecombo->Hide();
            m_timer_edit->Start(30000, true);
        });
    }

    void onLeftDClick(wxMouseEvent& evt)
    {        
        // 获取点击的行和列
        int  flag = 0;
        m_editRow = m_listCtrl->HitTest(evt.GetPosition(), flag, &m_editCol);
        if (m_editRow == -1 || m_editCol == -1) {
            evt.Skip();
            return;
        }
        //cout << "Edit cell at row: " << m_editRow << ", col: " << m_editCol << "flag:" << flag <<endl;
        wxRect rect;
        m_listCtrl->GetSubItemRect(m_editRow, m_editCol, rect);
        if (m_editCol == 5) { 
            if (m_listCtrl->GetItemText(m_editRow, 3) == "standby" ) {
                SelectAllFileFromPrint(rect);
            }
        } 
        else {
            if (m_editor) {
                m_editor->Destroy();
            }
            m_editor = new wxTextCtrl(m_listCtrl, wxID_ANY, m_listCtrl->GetItemText(m_editRow, m_editCol), rect.GetTopLeft(),
                                      rect.GetSize(), wxTE_PROCESS_ENTER);
            m_editor->SetFocus();
            m_editor->SetSelection(-1, -1); // 全选文本
            // 绑定编辑完成事件（回车或失去焦点）
            m_editor->Bind(wxEVT_TEXT_ENTER, &FarmManager::OnEditComplete, this);
            m_editor->Bind(wxEVT_KILL_FOCUS, &FarmManager::OnEditComplete, this);
        }
        evt.Skip();
    }
    
    void OnResultUpdated(wxCommandEvent& event)
    {
        int evtid = event.GetId();
        if (evtid == PRINTER_PING) {
            wxString result = event.GetString();
            if (result != "") {
                AddListPrinter(PrintIndex, "defult", result);                
                PrintIndex++;
            }
            CreateThredFunPing(); 
        }
        else if (event.GetId() == PRINTER_STATUS) {
            if (isActive) {
                wxString result = event.GetString();
                int      id     = event.GetInt();
                UpdatePrinter(id, result);
                if (stepQuery < IpList.size())
                    CreateThredFunQuery();
            }
        }else
            cout << event.GetString() << ", unknow id: " << evtid << endl;

    }
    
    void OnSearchPrinter(wxCommandEvent& event)
    {       
        IpList.clear();
        isActive   = false;
        stepPing   = 1;
        PrintIndex = 0;
        //stepQuery  = 0x7fffff;
        m_listCtrl->DeleteAllItems();
        wxStringTokenizer tokenizer(m_inputctrl->GetValue(), ".");
        if (tokenizer.CountTokens() > 2) {
            for (int i = 0; i < 3; i++) {
                tokenizer.GetNextToken().ToLong(ip + i);
                if (ip[i] < 0 || ip[i] > 255 || ip[0] == 0) {
                    wxMessageBox("Please enter a valid IP address.", "Invalid IP", wxOK | wxICON_ERROR);
                    return;
                    }
            }          
        }
        for (int i = 0; i < 16; i++) {
            CreateThredFunPing();
        }
    }
    
    void OnCommandSelect(wxCommandEvent& event)
    {
        int      evtid     = event.GetId() - FARM_ID_BASE;
        wxString currentIp = m_listCtrl->GetItemText(m_editRow, idx_COL_IPADDR);
        cout << "Printer itemIdx:" << m_editRow << ",id" << evtid << endl;
        wxString response;
        if (evtid >= 1080 && evtid < 1080 + 129)
        {
            wxString File_Name = m_listCtrl->GetItemText(m_editRow, idx_COL_INFO);
            if (evtid==1080)
                cout << "Sync to all group files:" << File_Name << endl;
            else
                cout << "Sync files to:group-" << GroupList[idx_COL_GROUP][evtid - 1081] << endl;
            for (long id = 0; id < m_listCtrl->GetItemCount();id++)
            {
                if (m_listCtrl->GetItemText(id, idx_COL_STATUS) == "standby") {
                    if (evtid == 1080 || GroupList[idx_COL_GROUP][evtid - 1081] == m_listCtrl->GetItemText(id, idx_COL_GROUP)) {
                        wxArrayString filelist;
                        ReadFileList(m_listCtrl->GetItemText(id, idx_COL_IPADDR), &filelist);
                        if (std::find(filelist.begin(), filelist.end(), File_Name) == filelist.end()) {
                            m_listCtrl->SetItemTextColour(id, wxColour("#800000"));
                            m_listCtrl->SetItem(id, idx_COL_INFO, "Error:No pushed files");
                        }
                        else
                            m_listCtrl->SetItem(id, idx_COL_INFO, File_Name);
                    }
                }
            }
            m_timer_edit->Start(30000, true);

        } else {
            switch (evtid) {
            case 1001:
                // m_listCtrl->DeleteItem(itemIdx);
                // IpList.erase(IpList.begin() + itemIdx);
                {
                    wxString Ip = m_listCtrl->GetItemText(m_editRow, idx_COL_IPADDR);
                    cout << "Remove Printer:" << m_listCtrl->GetItemText(m_editRow, idx_COL_IPADDR) << endl;
                    for (auto it = IpList.begin(); it != IpList.end();) {
                        if (it->Ip == Ip) {
                            IpList.erase(it);
                        } else
                            ++it;
                    }
                    m_listCtrl->DeleteItem(m_editRow);
                }
                break;
            case 1002: {
                wxString url = wxString::Format(m_Cancel_url, currentIp);
                HttpJsonClient::SendPostRequest(url, "", "application/json", response);
                cout << wxString::Format("Cancel:%s,result:%s\n", currentIp, response);
            }
            // cout << "Canel Print:" << m_listCtrl->GetItemText(m_editRow, idx_COL_IPADDR) << endl;
            break;
            case 1003: {
                //"/printer/print/pause"
                wxString url = wxString::Format(m_Pause_url, currentIp);
                HttpJsonClient::SendPostRequest(url, "", "application/json", response);
                cout << wxString::Format("Pause:%s,result:%s\n", currentIp, response);
            } break;
            case 1004: {
                wxString url = wxString::Format(m_Resume_url, currentIp);
                HttpJsonClient::SendPostRequest(url, "", "application/json", response);
                cout << wxString::Format("Resume:%s,result:%s\n", currentIp, response);
            }
            // cout << "Resume Print:" << m_listCtrl->GetItemText(m_editRow, idx_COL_INFO) << endl;
            break;
            case 1005: {
                wxString url      = wxString::Format(m_Print_url, currentIp);
                wxString fileName = m_listCtrl->GetItemText(m_editRow, idx_COL_INFO);
                wxString poststr  = wxString::Format("{\"filename\": \"%s\"}", fileName);
                HttpJsonClient::SendPostRequest(url, poststr, "application/json", response);
                cout << wxString::Format("Print:%s,%s,result:%s\n", currentIp, fileName, response);
            } break;
            case 1006: {
                wxString url     = wxString::Format("http://%s/printer/gcode/script", currentIp);
                wxString poststr = "{\"script\": \"G28\"}";
                HttpJsonClient::SendPostRequest(url, poststr, "application/json", response);
                cout << wxString::Format("HomeXYZ:%s,result:%s\n", currentIp, response);
            } break;
            case 1007: {
                // cout << "Sync files:" << m_listCtrl->GetItemText(m_editRow, idx_COL_INFO) << endl;
                wxRect rect;
                m_listCtrl->GetSubItemRect(m_editRow, idx_COL_INFO, rect);
                SelectAllFileFromPrint(rect);
            } break;
            default: break;
            }
        }
    }
    
    void onRightUp(wxListEvent& evt)
    {
        m_editRow = evt.GetIndex();
        m_editCol = evt.GetColumn();
        cout << wxString::Format("onRightUp Row=%d,Col=%d\n", m_editRow, m_editCol);
        //int flag;
        //m_editRow = m_listCtrl->HitTest(evt.GetPosition(), flag, &m_editCol);
        if (m_editRow != -1) {
            wxMenu* m_menu = new wxMenu();
            m_menu->Append(FARM_ID_BASE + 1001, "Remove Printer");
            if (m_listCtrl->GetItemText(m_editRow, idx_COL_STATUS) == "printing") {
                m_menu->Append(FARM_ID_BASE + 1002, "Canel Print");
                m_menu->Append(FARM_ID_BASE + 1003, "Paush Print");
            } // paused
            else if (m_listCtrl->GetItemText(m_editRow, idx_COL_STATUS) == "paused") {
                m_menu->Append(FARM_ID_BASE + 1002, "Canel Print");
                m_menu->Append(FARM_ID_BASE + 1004, "Resume Print");
            } else if (m_listCtrl->GetItemText(m_editRow, idx_COL_STATUS) == "standby") {
                m_menu->Append(FARM_ID_BASE + 1006, "Home XYZ");
                wxString tmpFile = m_listCtrl->GetItemText(m_editRow, idx_COL_INFO);
                if (tmpFile.EndsWith(".gcode")){
                //if (m_listCtrl->GetItemText(m_editRow, idx_COL_INFO) != "") {
                    //GetGroup(idx_COL_GROUP);
                    m_menu->Append(FARM_ID_BASE + 1005, "Print");                   
                    wxMenu* subMenu = new wxMenu();
                    // this, FARM_ID_BASE + 1005, "Sync files"
                    subMenu->Append(FARM_ID_BASE + 1080, "Sync to all group");
                    for (int i = 0; i < GroupList[idx_COL_GROUP].size(); i++) {
                        subMenu->Append(FARM_ID_BASE + 1081 + i, wxString::Format("Group<%s>", GroupList[idx_COL_GROUP][i]));
                    }
                    m_menu->AppendSubMenu(subMenu, "Sync files to standby");
                    
                } else
                    m_menu->Append(FARM_ID_BASE + 1007, "Select files from printer");
            }
            PopupMenu(m_menu, this->ScreenToClient(wxGetMousePosition()));
            delete m_menu;
        }
    }
};
//$ SAVE_VARIABLE VARIABLE=box_modify_t0 VALUE=0
//$ SAVE_VARIABLE VARIABLE=box_modify_t1 VALUE=1
//$ SAVE_VARIABLE VARIABLE=box_modify_t2 VALUE=2
//$ SAVE_VARIABLE VARIABLE=box_modify_t3 VALUE=3
class BrowserTabPanel : public wxPanel {
public:
    wmStatusPanel* m_statusbar;
    BrowserTabPanel(wxWindow* parent)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize) //, wxHSCROLL | wxVSCROLL | wxTAB_TRAVERSAL)
    { 
        Bind(wxEVT_SOCKET, &BrowserTabPanel::OnSocketEvent, this);
        Bind(wxEVT_BUTTON, &BrowserTabPanel::OnCustomEvent, this);
        Bind(wxEVT_SIZE, &BrowserTabPanel::OnSizeChange, this);
        Bind(wxEVT_TIMER, &BrowserTabPanel::OnTimer, this);
        //Bind(wxEVT_ACTIVATE, &BrowserTabPanel::OnActivate, this);
        
        wxDisplay mdisplay(wxDisplay::GetFromWindow(this));
        scaleX = mdisplay.GetScaleFactor();
        cout << "scaleX:" << scaleX << endl;

        m_timer = new wxTimer(this);      
        m_AppPath               = GetAppPath();
        wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);
        panel1Sizer = new wxBoxSizer(wxHORIZONTAL);
        wxBoxSizer* cameraSizer = new wxBoxSizer(wxVERTICAL);
        //m_Status                = new customPanel(this, wxID_ANY);
        //m_Status->Hide();
        panelR                  = new wxPanel(this, wxID_ANY);
        panel1                  = new wxPanel(panelR, wxID_ANY);
        panel2                  = new FileBrowserCtrl(panelR, m_AppPath);
        panel3                  = new FarmManager(panelR, m_AppPath);
        panel4                  = new wxPanel(panelR, wxID_ANY);
        wxPanel*    leftPanel   = new wxPanel(this, wxID_ANY);
        IpListBox               = new wxComboBox(leftPanel, wxID_ANY, "   .   .   .   ", wxDefaultPosition, wxDefaultSize, host_Ip_list);
        IpListBox->SetHint("Type Ip");
        wxButton*   butonGo  = new wxButton(leftPanel, wxID_ANY, "+");
        wxBoxSizer* SelectIp = new wxBoxSizer(wxHORIZONTAL);
        SelectIp->Add(IpListBox, 1, wxEXPAND | wxTOP | wxBOTTOM, 10);
        SelectIp->Add(butonGo, 0, wxEXPAND | wxTOP | wxBOTTOM, 10);
        butonGo->Bind(wxEVT_BUTTON, [this](wxCommandEvent& evt) {
            wxString rusult;
            wxString textIp = IpListBox->GetValue();
            wxString url    = wxString::Format("http://%s/printer/info", IpListBox->GetValue());
            if (HttpJsonClient::GetPrinterJson(url, &rusult)) {
                SynchronizeIP(textIp);
                if (IpListBox->FindString(textIp)<0)
                    IpListBox->Insert(textIp, 0);
            }
        });
        //wxTextCtrl* textip      = new wxTextCtrl(leftPanel, wxID_ANY, "");
        wxButton*   buton1      = new wxButton(leftPanel, wxID_ANY, "Status");
        wxButton*   buton2      = new wxButton(leftPanel, wxID_ANY, "Storage");
        wxButton*   buton3      = new wxButton(leftPanel, wxID_ANY, "Hidden");
        wxButton*   buton4      = new wxButton(leftPanel, wxID_ANY, "Assistant");
        wxBoxSizer* sidebar     = new wxBoxSizer(wxVERTICAL);
        butonGo->SetMinSize(wxSize(30, 30));
        buton1->SetMinSize(wxSize(144, 50));
        buton2->SetMinSize(wxSize(144, 50));
        buton3->SetMinSize(wxSize(144, 50));
        buton4->SetMinSize(wxSize(144, 50));
        sidebar->Add(SelectIp, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
        sidebar->Add(buton1, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
        sidebar->Add(buton2, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
        sidebar->Add(buton3, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
        sidebar->Add(buton4, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
        leftPanel->SetSizer(sidebar);
       
        buton1->Bind(wxEVT_BUTTON, [this](wxCommandEvent& evt) {
            Print_webview->Stop();
            panel2->Hide();
            panel3->Hide();
            panel4->Hide();
            panel1->Show();
            mR_Sizer->Clear();
            mR_Sizer->Add(panel1, 1, wxEXPAND | wxALL);
            mR_Sizer->Layout();
            });
        buton2->Bind(wxEVT_BUTTON, [this](wxCommandEvent& evt) {
            panel1->Hide();
            panel3->Hide();
            panel4->Hide();
            panel2->Show();
            mR_Sizer->Clear();
            mR_Sizer->Add(panel2, 1, wxEXPAND | wxALL);
            mR_Sizer->Layout();
            if (hostIp != "" && !isFileShow) {
                panel2->SetHostIp(hostIp);
                isFileShow = true;
            }
        });
        buton3->Bind(wxEVT_BUTTON, [this](wxCommandEvent& evt) {
            panel2->Hide();
            panel1->Hide();
            panel4->Hide();
            panel3->Show();
            mR_Sizer->Clear();
            mR_Sizer->Add(panel3, 1, wxEXPAND | wxALL);
            mR_Sizer->Layout();
            panel3->SetActive(true);
        });
        buton4->Bind(wxEVT_BUTTON, [this](wxCommandEvent& evt) {
            panel2->Hide();
            panel3->Hide();
            panel1->Hide();
            panel4->Show();
            Print_webview->LoadURL(wxString::Format("http://%s", hostIp));
            mR_Sizer->Clear();
            mR_Sizer->Add(panel4, 1, wxEXPAND | wxALL);
            mR_Sizer->Layout();
        });

        wxBoxSizer* panel4Sizer = new wxBoxSizer(wxVERTICAL);
        Print_webview = wxWebView::New(panel4, wxID_ANY);
        //Print_webview->SetMinSize(wxSize(800, 600));
        panel4Sizer->Add(Print_webview,1, wxEXPAND | wxALL, 5);
        panel4->SetSizer(panel4Sizer);

        wxString  choices[]    = {"Camera", "Refresh", "Close"};

        m_CameraMenu = new wxChoice(panel1, wxID_ANY, wxDefaultPosition, wxDefaultSize, 3, choices);
        m_CameraMenu->SetSelection(0);
        m_CameraMenu->Bind(wxEVT_CHOICE, &BrowserTabPanel::OnChoiceSelect, this);
        m_webview = wxWebView::New(panel1, wxID_ANY);
        m_webview->SetMinSize(wxSize(320, 240)); 
        //m_webview->SetZoom(wxWEBVIEW_ZOOM_LARGEST);
        m_webview->Bind(wxEVT_WEBVIEW_ERROR, &BrowserTabPanel::OnWebError, this); // 加载错误
        m_CameraInfo = new wxStaticText(panel1, wxID_ANY, "Camera not connected");

        wxBoxSizer* cameraTop = new wxBoxSizer(wxHORIZONTAL);
        cameraTop->Add(m_CameraMenu);
        cameraTop->Add(m_CameraInfo);
        cameraSizer->Add(cameraTop,0,wxEXPAND);
        cameraSizer->Add(m_webview,1,wxEXPAND);

        m_statusbar = new wmStatusPanel(panel1, wxID_ANY );
        txtGcode    = new wxTextCtrl(panel1, wxID_ANY, "12345678901234567890" );
        SendGcode   = new wxButton(panel1, wxID_ANY, "Send" );
        btRestart   = new wxButton(panel1, wxID_ANY, "Restart");
        wxBoxSizer* GcodeSize = new wxBoxSizer(wxHORIZONTAL);
        GcodeSize->Add(btRestart, 0, wxEXPAND);
        GcodeSize->Add(txtGcode, 1, wxEXPAND );
        GcodeSize->Add(SendGcode, 0, wxEXPAND| wxRIGHT,10); 

        wxBoxSizer* barSize = new wxBoxSizer(wxVERTICAL);
        barSize->Add(GcodeSize, 0, wxEXPAND | wxRIGHT, 10); 
        barSize->Add(m_statusbar, 0, wxEXPAND | wxRIGHT, 10); 

        panel1Sizer->Add(barSize,0, wxEXPAND);
        panel1Sizer->Add(cameraSizer, 1, wxEXPAND);
        panel1->SetSizer(panel1Sizer);
     
        txtGcode->SetHint("Type Gcode");
        SendGcode->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            //wxString gcode = txtGcode->GetValue();
            char        buffer[256];
            std::string utf8_msg = txtGcode->GetValue().ToUTF8().data();
            snprintf(buffer, sizeof(buffer), "{\"script\": \"%s\"}", utf8_msg.c_str());
            SocketSendRPC("printer.gcode.script", buffer, 1200);
        });        
        btRestart->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            SocketSendRPC("printer.firmware_restart", "null", 1300); });
      
        mR_Sizer = new wxBoxSizer(wxVERTICAL);
        panel2->SetBackgroundColour(wxColour(255, 248, 240)); // 浅橙
        panel3->SetBackgroundColour(wxColour(240, 255, 248)); // 浅绿
        panel4->SetBackgroundColour(wxColour(240, 248, 255)); // 浅蓝
        panel2->Hide();
        panel3->Hide();
        panel4->Hide();
        TestLastPrinter();
        //panelR->SetMinSize(wxSize(800, 600));
        //panel1->SetMinSize(wxSize(800, 600));
        //panel2->SetMinSize(wxSize(800, 600));
        //panel3->SetMinSize(wxSize(800, 600));
        //panel4->SetMinSize(wxSize(800, 600));
        mR_Sizer->Add(panel1 ,1, wxEXPAND | wxALL);
        panelR->SetSizer(mR_Sizer);

        mainSizer->Add(leftPanel, 0, wxEXPAND , 10);
        mainSizer->Add(panelR, 1, wxEXPAND | wxALL, 10);
        this->SetSizer(mainSizer);
        mainSizer->Fit(this);
        //mR_Sizer->Fit(panelR);
        mainSizer->SetSizeHints(this);
    }
   
    void TestLastPrinter() {
        fs::path file_path = m_AppPath.ToStdString() + "/resources/info/printer.txt";

        //wxArrayString host_Ip_list;
        host_Ip_list.Add("");
        if (!HttpJsonClient::read_text_file(file_path, &host_Ip_list)) {           
            PresetBundle*      preset_bundle = wxGetApp().preset_bundle;
            DynamicPrintConfig cfg           = preset_bundle->printers.get_edited_preset().config;
            string             hostprint     = cfg.opt_string("print_host");
            host_Ip_list.Item(0)             = hostprint;
            cout << "read ip from app\n";
        }
        for (auto tmp : host_Ip_list) {
            IpListBox->Append(tmp);
        }
        wxString rusult;
        wxString url = wxString::Format("http://%s/printer/info", host_Ip_list.Item(0));
        if (HttpJsonClient::GetPrinterJson(url, &rusult)) {
            SynchronizeIP(host_Ip_list.Item(0));
            IpListBox->SetValue(host_Ip_list.Item(0));
        }
        //m_Status->SetIpText(host_Ip_list.Item(0), host_Ip_list);
    }
    void OnActivate() { 
        thisActive = true;
        txtGcode->SetLabelText("");
        //CreatSocket();
        if (hostIp == "") {
            //fs::path      file_path = "E:\\new_orca\\OrcaSlicer\\build\\OrcaSlicer\\resources\\info\\printer.txt";
            TestLastPrinter();
        } else {
            SynchronizeIP(hostIp);
            //ConnectServer(hostIp, 7125);
        }
    }
    void UnActivate() { 
        thisActive = false;
        if (m_socket!=nullptr) {
            m_socket->Close();
            //m_socket = nullptr;
        }
    }
    #if 0
    static std::string GetAppPath() {
        std::string full_path;
#if defined(_WIN32) || defined(_WIN64)
        // Windows：使用 GetModuleFileNameA（ANSI 版本，对应 std::string）
        std::vector<char> buf(MAX_PATH);
        while (true) {
            DWORD len = GetModuleFileNameA(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
            if (len == 0) {
                throw std::runtime_error("GetModuleFileNameA failed, error code: " + std::to_string(GetLastError()));
            }
            if (len < buf.size()) {
                full_path.assign(buf.data(), len);
                break;
            }
            buf.resize(buf.size() * 2);
        }

#elif defined(__linux__)
        // Linux：读取 /proc/self/exe 符号链接
        std::vector<char> buf(1024);
        while (true) {
            ssize_t len = readlink("/proc/self/exe", buf.data(), buf.size() - 1); // 留1字节存'\0'
            if (len == -1) {
                throw std::runtime_error("readlink failed: " + std::string(strerror(errno)));
            }
            if (static_cast<size_t>(len) < buf.size() - 1) {
                full_path.assign(buf.data(), len);
                break;
            }
            // 缓冲区不足，翻倍扩容
            buf.resize(buf.size() * 2);
        }

#elif defined(__APPLE__)
        // macOS：_NSGetExecutablePath + realpath 转换绝对路径
        char     path_buf[PATH_MAX];
        uint32_t buf_len = PATH_MAX;
        int      ret     = _NSGetExecutablePath(path_buf, &buf_len);
        // 缓冲区不足时扩容
        if (ret == -1) {
            std::vector<char> big_buf(buf_len);
            ret = _NSGetExecutablePath(big_buf.data(), &buf_len);
            if (ret != 0) {
                throw std::runtime_error("NSGetExecutablePath failed, code: " + std::to_string(ret));
            }
            // realpath 转换为绝对路径
            char abs_path[PATH_MAX];
            if (realpath(big_buf.data(), abs_path) == nullptr) {
                throw std::runtime_error("realpath failed: " + std::string(strerror(errno)));
            }
            full_path = abs_path;
        } else {
            // 缓冲区足够，直接转换为绝对路径
            char abs_path[PATH_MAX];
            if (realpath(path_buf, abs_path) == nullptr) {
                throw std::runtime_error("realpath failed: " + std::string(strerror(errno)));
            }
            full_path = abs_path;
        }

#endif
        size_t last_slash_pos;
#if defined(_WIN32) || defined(_WIN64)
        // Windows 路径分隔符：\（注意转义），同时兼容 /（部分场景可能出现）
        last_slash_pos = full_path.find_last_of("\\/");
#else
        // Linux/macOS 路径分隔符：/
        last_slash_pos = full_path.find_last_of('/');
#endif

        if (last_slash_pos == std::string::npos) {
            // 返回当前目录 "." 或根目录 "/"，避免返回空字符串
            return full_path.empty() ? "." : full_path;
        }
        // 截取目录部分（从开头到最后一个分隔符）
        return full_path.substr(0, last_slash_pos);
    }
    #else
    static wxString GetAppPath() {

        std::wstring path(size_t(MAX_PATH_Len), wchar_t(0));
        int          len = int(::GetModuleFileName(nullptr, path.data(), MAX_PATH_Len));
        if (len > 0 && len < MAX_PATH_Len) {
            path.erase(path.begin() + len, path.end());
        }
        wxFileName mfileName(path);
        return mfileName.GetPath();
    }
    #endif
    void SynchronizeIP(wxString msg) {
        //cout << "update Ip:" << msg << endl;
        hostIp = msg;
        SendIpToOrca(hostIp);
        IpListBox->SetToolTip(hostIp);
        if (thisActive) {
            m_statusbar->SetHostIp(hostIp);
            if (panel2->IsShown())
                panel2->SetHostIp(hostIp);
            else
                isFileShow = false;
            if (hostIp == "") {
                m_CameraInfo->SetLabelText("Printer not connected");
                m_webview->LoadURL("about:blank");
                m_webview->Hide();
                if (m_socket->IsConnected())
                    m_socket->Close();
            } else {
                m_webview->Show();
                m_CameraInfo->SetLabelText("Camera connecting");
                wxString url = wxString::Format("http://%s/webcam/?action=stream", hostIp);
                m_webview->LoadURL(url);
                ConnectServer(hostIp);
            }
        }
    }
    ~BrowserTabPanel()
    {
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " Start";
        SetEvtHandlerEnabled(false);
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " End";
    }
    
 private:
    wxString        hostIp         = "";
    wxBoxSizer*     m_sizer;
    wxBoxSizer*     mR_Sizer;
    wxBoxSizer*     panel1Sizer;
    wxPanel*         panelR;
    wxPanel*        panel1;
    FileBrowserCtrl* panel2;
    FarmManager*    panel3;
    wxPanel*        panel4;
    //customPanel*    m_Status;
    wxWebView*      m_webview;
    wxChoice*       m_CameraMenu;
    wxStaticText*   m_CameraInfo;
    wxSocketClient* m_socket   = nullptr;
    wxTimer*        m_timer    = nullptr;
    bool            thisActive = false;
    bool            isFileShow = false;
    vector<jsonrpcInfo> jsonrpclist;
    wxTextCtrl*         txtGcode;
    wxButton*           SendGcode;
    wxButton*           btRestart;    //wxBitmapButton
    wxArrayString       host_Ip_list;
    wxComboBox*         IpListBox;
    wxWebView*          Print_webview;
    wxString            m_AppPath;
    double              scaleX = 1.0;

    void SendIpToOrca(wxString msg) {
        PresetBundle*      preset_bundle = wxGetApp().preset_bundle;
        DynamicPrintConfig cfg           = preset_bundle->printers.get_edited_preset().config;
        string             hostprint     = cfg.opt_string("print_host");
        cout << "App ip=" << hostprint << endl;
        hostprint = msg.ToUTF8();
        cfg.set_key_value("print_host", new ConfigOptionString(hostprint));
        preset_bundle->printers.get_edited_preset().config = cfg;
    }

    void OnCustomEvent(wxCommandEvent& event)
    {
        // 从事件中获取数据
        wxString msg = event.GetString();
        int      id      = event.GetInt();
        // 处理事件
        std::cout << "Received custom event: message=" << msg << ", id=" << id << std::endl;
        /*if (msg == "Tab_Change") {
            chageTab(id);
        }*/
        if (id == 810)  // send gcode
        {
            if (hostIp != "") {
                //HttpJsonClient::PostGocde(hostIp, msg);
                char buffer[256];
                std::string utf8_msg = msg.ToUTF8().data();
                snprintf(buffer, sizeof(buffer), "{\"script\": \"%s\"}", utf8_msg.c_str());
                //wxString buffer = wxString::Format("{\"script\": \"%s\"}", msg);
                //cout << "send gcode:" << buffer << endl;
                SocketSendRPC("printer.gcode.script", buffer, 1100);
            }
        }
        if (id == 999) { // connect suscess
            SynchronizeIP(msg);
            //wxWebView* m_web1  = (wxWebView*) panel1->GetWindowChild(1);                           
        }
        event.Skip();
    }
  
   
    void OnSizeChange(wxSizeEvent& evt) 
    {
        int targetOrientation = (this->GetClientSize().GetWidth() / scaleX < 900) ? wxVERTICAL : wxHORIZONTAL;
        if (panel1Sizer->GetOrientation() != targetOrientation) {
            panel1Sizer->SetOrientation(targetOrientation);
            bool m_webviewisshow = m_webview->IsShown();
            if (!m_webviewisshow)
                m_webview->Show();
            panel1->Layout();           // 刷新panel1的布局
            if (!m_webviewisshow)
                m_webview->Hide();
            this->GetSizer()->Layout(); // 刷新主sizer的布局（确保整体适配）
            // this->FitInside();
        }
        evt.Skip();
    }
    
    void OnWebError(wxWebViewEvent& event)
    {
        wxString errorMsg = "Error:" + event.GetString(); // 错误信息
        m_webview->Hide();
        m_CameraInfo->SetLabelText(errorMsg);
        //cout << "WebView Load Error: " << errorMsg << endl;
    }
    
    void OnChoiceSelect(wxCommandEvent& event)
    {
        int selection = m_CameraMenu->GetSelection(); // 获取选中的索引
        if (selection == wxNOT_FOUND)
            return;
        else if (selection == 0) { // Camera
            // choice->SetSelection(0);
            cout << "Camera Selected" << endl;
        } else if (selection == 1) { // Refresh
            if (hostIp != "") {
                m_webview->Show();
                m_CameraInfo->SetLabelText("Camera connecting");
                wxString url = wxString::Format("http://%s/webcam/?action=stream", hostIp);
                m_webview->LoadURL(url);
            } else
                m_CameraInfo->SetLabelText("Printer not connected");
            m_CameraMenu->SetSelection(0);
        } else if (selection == 2) { // Close
            m_webview->Hide();
            m_CameraInfo->SetLabelText("Camera closed");
            m_webview->LoadURL("about:blank");
            m_CameraMenu->SetSelection(0);
        }
    }

    wxButton* m_side_tools;
    Tabbook*   m_tabpanel;
    
    void ConnectServer(wxString ip,int port=7125) {
        wxIPV4address addr;
        addr.Hostname(ip);
        addr.Service(port);
        if (m_socket == nullptr){
            m_socket = new wxSocketClient(wxSOCKET_NOWAIT); // 创建非阻塞 Socket
            m_socket->SetEventHandler(*this, wxID_ANY);
            m_socket->SetNotify(wxSOCKET_CONNECTION_FLAG | wxSOCKET_INPUT_FLAG | wxSOCKET_LOST_FLAG | wxSOCKET_OUTPUT_FLAG);
            m_socket->Notify(true);
            m_socket->SetTimeout(5);
        } else {
            m_socket->Close();
        }
        // Connect() 是异步的，会立即返回
        if (!m_socket->Connect(addr, true)) // false 表示不等待连接完成
        {
            std::cout << "Connect Fail\r\n";
            return;
        } else
            std::cout << "Connect...\r\n";
        // 构建 HTTP GET 请求
        m_socket->SetFlags(wxSOCKET_WAITALL);
        wxString request = wxString::Format("GET %s HTTP/1.1\r\nHost: %s\r\n"
                                            "Upgrade: websocket\r\n"
                                            "Connection: Upgrade\r\n"
                                            "Sec-WebSocket-Key: %s\r\n"
                                            "Sec-WebSocket-Version: 13\r\n\r\n",
                                            "/websocket", "192.168.1.159:7125", "dGhlIHNhbXBsZSBub25jZQ==");
        m_socket->Write(request.c_str(), request.Length()); // 添加换行符作为消息分隔符
        char     buffer[2048];
        wxUint32 bytesRead = m_socket->Read(buffer, sizeof(buffer)).LastCount();
       // wxString receivedData(buffer, bytesRead);
        //std::cout << "receive:" << buffer << endl;

        char* sendinfo1 = "{\"jsonrpc\": \"2.0\", \"method\": \"server.connection.identify\", "
                          "\"params\": {\"client_name\": \"orca\", \"version\": \"1.0.0\", \"type\": "
                          "\"web\", \"url\": \"https://github.com/eez_test\"}, \"id\": 444}";
        // m_socket->SetTimeout(5);
        vector<uint8_t> frame1 = WebSocket_m::EncodeWebSocketFrame(sendinfo1, strlen(sendinfo1));
        m_socket->Write(frame1.data(), frame1.size());
        bytesRead = m_socket->Read(buffer, sizeof(buffer)).LastCount();
        m_socket->SetFlags(wxSOCKET_NOWAIT);
        //std::cout << "receive:" << buffer << endl;
        //SET_FAN_SPEED FAN = auxiliary_fan SPEED = 0.5  //air_fan 
        
        char* objects = "{\"jsonrpc\": \"2.0\", \"method\": \"printer.objects.subscribe\", \"params\""
                            ": {\"objects\": {"
                            "\"print_stats\": [\"state\", \"filename\"], "
                            "\"display_status\": [\"progress\"], "
                            "\"toolhead\": [\"homed_axes\",\"position\",\"extruder\"], "
                            "\"fan\": null, \"controller_fan motherboard_fan\": null, "
                            "\"fan_generic air_fan\": null, \"fan_generic auxiliary_fan\": null, " 
                            "\"heater_fan hotend_fan\": null, "
                            "\"heater_fan hotend0_fan\": null, \"heater_fan hotend1_fan\": null, "
                            "\"heater_fan hotend2_fan\": null, \"heater_fan hotend3_fan\": null, "
                            "\"neopixel T0_RGB\": null, \"neopixel T1_RGB\": null, "
                            "\"neopixel T2_RGB\": null, \"neopixel T3_RGB\": null, "
                            "\"extruder\": [\"temperature\", \"target\", \"power\"], "
                            "\"extruder1\": null, \"extruder2\": null, \"extruder3\": null, "
                            "\"heater_bed\": [\"temperature\", \"target\", \"power\"]}}, \"id\": 444}";
        /*SocketSendRPC("{\"jsonrpc\": \"2.0\",\"method\": \"printer.objects.subscribe\",\"params\": {\"objects\": {\"gcode_move\": "
                      "[\"position\",\"speed\"], \"toolhead\": [\"position\", \"status\"]}},\"id\": 5434}");*/
        //wxString        subscribe_msg = wxString::Format(objects, "printer.objects.query", 555);
        vector<uint8_t> frame = WebSocket_m::EncodeWebSocketFrame(objects, strlen(objects));
        m_socket->Write(frame.data(), frame.size());
    
    }

    void OnTimer(wxTimerEvent& event)
    {
        if (jsonrpclist.size() > 0) {
            std::cout << "timeout id:" << jsonrpclist[0].id << ",method:" << jsonrpclist[0].method << ",text:" << jsonrpclist[0].text
                      << endl;
            jsonrpclist.erase(jsonrpclist.begin());
            if (jsonrpclist.size() > 0)
                m_timer->Start(500, wxTIMER_ONE_SHOT);
        } 
    }

    void SocketSendRPC(char* method,char* params,int id,bool cheak = true)
    {
        jsonrpcInfo jrpc = {id, method, params, 0, "packet loss"};
        /*if (jrpc.text != "") {
            jsonrpclist.push_back(jrpc);
            if (m_timer && !m_timer->IsRunning())
                m_timer->Start(500, wxTIMER_ONE_SHOT);
        }*/
        if (!m_socket->IsConnected() && hostIp != "") {
            std::cout << "m_socket not connected:" << m_socket->LastError() << endl;
            ConnectServer(hostIp);
            return;
        } 
        char sendinfo[1024];
        snprintf(sendinfo, 1024, "{\"jsonrpc\": \"2.0\",\"method\": \"%s\",\"params\": %s,\"id\": %d}", method, params, id);
        vector<uint8_t>frame = WebSocket_m::EncodeWebSocketFrame(sendinfo, strlen(sendinfo));
        //cout << "sendinfo:\r\n" << sendinfo << endl;
        m_socket->Write(frame.data(), frame.size());
        if (cheak) {
            jsonrpclist.push_back(jrpc);
            if (m_timer && !m_timer->IsRunning())
                m_timer->Start(500, wxTIMER_ONE_SHOT);
        }
    }
  
    void OnSocketEvent(wxSocketEvent& event)
    {
        // 检查事件类型
        wxString request;
        switch (event.GetSocketEvent()) {
        case wxSOCKET_CONNECTION:
            // 连接成功
            std::cout<< "connect success!\r\n"; 
            //request = wxString::Format("GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: wxWidgets wxSocketClient\r\nAccept: */*\r\n\r\n",
            //                            "/printer/info", "192.168.1.159:7125");
           // m_socket->Write(request.c_str(), request.Length());
            //SocketSendwxString();
            break;
        case wxSOCKET_INPUT:
            {                
                char     buffer[4096];
                wxUint32 bytesRead = m_socket->Read(buffer, sizeof(buffer)).LastCount();
                if (bytesRead >= sizeof(buffer)) {
                    bytesRead         = sizeof(buffer) - 1;
                    buffer[bytesRead] = 0;
                }
                if (bytesRead > 0) {
                    WebSocketFrameHeader header;
                    if (WebSocket_m::DecodeWebSocketFrameHeader(buffer, bytesRead, header)) {
                        /*wxString receivedData = wxString::Format("%02x%02x%02x%02x,Total:%d,h:%d,s:%d,t:%d\r\n", buffer[0], buffer[1],
                                                                 buffer[2], buffer[3], bytesRead, header.h_len, header.p_len,
                                                                 header.opcode);*/
                        //std::cout << receivedData;
                        if (header.opcode == 0x08) {
                            std::cout << "WebSocket connection closed by server." << std::endl;
                            //m_socket->Close();
                            //return;
                        } else if (header.opcode == 0x09) {
                            std::cout << "ping frame. Sending pong..." << std::endl;
                            // 回复 pong 帧
                            vector<uint8_t> pongFrame = WebSocket_m::EncodeWebSocketFrame(buffer + header.h_len,header.p_len, 0x0A);
                            m_socket->Write(pongFrame.data(), pongFrame.size());
                        }
                        else if (header.opcode == 0x01) {
                            std::string        str(reinterpret_cast<const char*>(buffer + header.h_len), header.p_len);
                            //std::string        str(buffer + header.h_len, header.p_len);                            
                            try {
                                json sjson = json::parse(str);
                                if (sjson.contains("id")) {
                                    int  id = (int) sjson.at("id");
                                    auto it = std::remove_if(jsonrpclist.begin(), jsonrpclist.end(), [id](const jsonrpcInfo& d) {
                                        return d.id == id; // 返回 true 表示这个元素要被 "移除"
                                    });
                                    jsonrpclist.erase(it, jsonrpclist.end());
                                    cout << "recive:" << id << endl;
                                }
                                if (sjson.contains("error")) {
                                    m_statusbar->UpdaePrintInfo(sjson);
                                }
                                else if (sjson.contains("result")) {
                                    if (sjson.at("result").contains("status"))
                                        m_statusbar->UpdaePrintInfo((json)sjson.at("result").at("status"));
                                } else if (sjson.contains("method")) {
                                    if (sjson["method"].get<std::string>() == "notify_status_update") {
                                        m_statusbar->UpdaePrintInfo((json) sjson.at("params").at(0));
                                    } else if (sjson["method"].get<std::string>() == "notify_proc_stat_update") {
                                        //if (sjson["params"][0].contains("moonraker_stats"))
                                        m_statusbar->SetCpuUsage(sjson["params"][0]["moonraker_stats"]["cpu_usage"].get<double>());
                                        //cout << "cpu_usage:" << sjson["params"][0]["moonraker_stats"]["cpu_usage"].get<double>() << endl;
                                    
                                    } else if (sjson["method"].get<std::string>() == "notify_filelist_changed") {
                                        cout << sjson["params"][0]["action"] << ":" << sjson["params"][0]["item"]["path"] << endl;    
                                    }
                                    else
                                        cout << "unknow1:" << wxString::FromUTF8(sjson["params"][0].dump()) << endl;
                                } else {
                                    cout << "unknow2:" << wxString::FromUTF8(str) << endl;
                                }
                            } catch (const std::exception& e) {
                                std::cout << "exception:" << e.what() << endl;
                                std::cout << str << endl;
                            }
                        }/*else
                            std::cout << receivedData << endl;*/
                    } else {
                        std::cout << "UnParse:" << buffer << endl;
                    }
                }
            }
            break;
        case wxSOCKET_LOST:
            // 连接丢失
            std::cout << "wxSOCKET_LOST" << endl;
            m_socket->Close();
            if (thisActive)
                ConnectServer(hostIp, 7125);
            break;
        }
    }
};

} // namespace GUI
} // namespace Slic3r