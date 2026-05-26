#include "updater.h"
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <curl/curl.h>
#include "../core/globals.h"
#include "../utils/crashlog.h"
#include "../utils/helpers.h"
#include "../node/server.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    std::string* s = reinterpret_cast<std::string*>(userp);
    s->append(reinterpret_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

static bool DownloadString(const std::string& url, std::string& outData)
{
    CURL* curl = curl_easy_init();
    if(!curl) return false;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outData);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return res == CURLE_OK;
}

static bool DownloadFile(const std::string& url, const std::filesystem::path& dest)
{
    CURL* curl = curl_easy_init();
    if(!curl) return false;
    FILE* fp = _wfopen(dest.c_str(), L"wb");
    if(!fp){
        curl_easy_cleanup(curl);
        return false;
    }
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    CURLcode res = curl_easy_perform(curl);
    fclose(fp);
    curl_easy_cleanup(curl);
    return (res == CURLE_OK);
}

// SHA256 using EVP API (OpenSSL 3 compatible — old SHA256_CTX is deprecated)
static std::string FileChecksum(const std::filesystem::path& filepath)
{
    std::ifstream file(filepath, std::ios::binary);
    if(!file) return "";

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if(!ctx) return "";

    if(EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }

    char buf[4096];
    while(file.read(buf, sizeof(buf))) {
        EVP_DigestUpdate(ctx, buf, (size_t)file.gcount());
    }
    if(file.gcount() > 0) {
        EVP_DigestUpdate(ctx, buf, (size_t)file.gcount());
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int  hashLen = 0;
    EVP_DigestFinal_ex(ctx, hash, &hashLen);
    EVP_MD_CTX_free(ctx);

    std::ostringstream oss;
    for(unsigned int i = 0; i < hashLen; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    return oss.str();
}

static bool VerifySignature(const std::string& data, const std::string& signatureBase64)
{
    BIO* bio = BIO_new_mem_buf(public_key_pem, -1);
    EVP_PKEY* pubKey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    if(!pubKey) return false;

    std::string cleanedSig;
    for(char c : signatureBase64){
        if(!isspace((unsigned char)c))
            cleanedSig.push_back(c);
    }

    BIO* b64  = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO* bmem = BIO_new_mem_buf(cleanedSig.data(), (int)cleanedSig.size());
    bmem = BIO_push(b64, bmem);

    std::vector<unsigned char> signature(512);
    int sig_len = BIO_read(bmem, signature.data(), (int)signature.size());
    BIO_free_all(bmem);
    if(sig_len <= 0) { EVP_PKEY_free(pubKey); return false; }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_PKEY_CTX* pctx = nullptr;
    bool result = false;
    if(EVP_DigestVerifyInit(ctx, &pctx, EVP_sha256(), nullptr, pubKey) == 1){
        if(EVP_DigestVerifyUpdate(ctx, data.data(), data.size()) == 1){
            result = (EVP_DigestVerifyFinal(ctx, signature.data(), sig_len) == 1);
        }
    }
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pubKey);
    return result;
}

void RunAutoUpdaterOnce()
{
    g_updaterRunning = true;
    std::cout << "[UPDATER]: Checking for updates...\n";

    std::string versionContent;
    if(!DownloadString(g_updateUrl, versionContent)) {
        AppendToCrashLog("[UPDATER]: Failed to download version.json");
        return;
    }
    nlohmann::json versionJson;
    try { versionJson = nlohmann::json::parse(versionContent); }
    catch(...) { return; }

    std::string versionDescUrl  = versionJson["versionDesc"].get<std::string>();
    std::string signatureBase64 = versionJson["signature"].get<std::string>();

    std::string detailsContent;
    if(!DownloadString(versionDescUrl, detailsContent)) {
        AppendToCrashLog("[UPDATER]: Failed to download version details");
        return;
    }
    if(!VerifySignature(detailsContent, signatureBase64)) {
        AppendToCrashLog("[UPDATER]: Signature verification failed");
        return;
    }
    nlohmann::json detailsJson;
    try { detailsJson = nlohmann::json::parse(detailsContent); }
    catch(...) { return; }

    std::string remoteShellVersion = detailsJson["shellVersion"].get<std::string>();
    bool needsFullUpdate = (remoteShellVersion != APP_VERSION);

    auto files = detailsJson["files"];
    std::vector<std::string> partialUpdateKeys = { "server.js" };

    wchar_t tmpBuf[MAX_PATH];
    GetTempPathW(MAX_PATH, tmpBuf);
    std::filesystem::path tempDir = std::filesystem::path(tmpBuf) / L"darkflix_updater";
    std::filesystem::create_directories(tempDir);

    if(needsFullUpdate || g_autoupdaterForceFull) {
        std::string key = "windows";
        SYSTEM_INFO si; GetNativeSystemInfo(&si);
        if(si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)        key = "windows-x64";
        else if(si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)   key = "windows-x86";

        bool allOk = false;
        if(files.contains(key) && files[key].contains("url") && files[key].contains("checksum")) {
            std::string url              = files[key]["url"].get<std::string>();
            std::string expectedChecksum = files[key]["checksum"].get<std::string>();
            std::string filename         = url.substr(url.find_last_of('/') + 1);
            std::filesystem::path installerPath = tempDir / std::wstring(filename.begin(), filename.end());

            for(const auto& entry : std::filesystem::directory_iterator(tempDir)) {
                if(entry.path() != installerPath) {
                    try { std::filesystem::remove_all(entry.path()); } catch(...) {}
                }
            }

            if(std::filesystem::exists(installerPath) &&
               FileChecksum(installerPath) == expectedChecksum) {
                allOk = true;
            } else {
                std::filesystem::remove(installerPath);
                allOk = DownloadFile(url, installerPath) &&
                        FileChecksum(installerPath) == expectedChecksum;
            }

            if(allOk) {
                g_installerPath = installerPath;
                nlohmann::json j; j["type"] = "requestUpdate";
                g_outboundMessages.push_back(j);
                PostMessage(g_hWnd, WM_NOTIFY_FLUSH, 0, 0);
                std::cout << "[UPDATER]: Full update ready.\n";
            } else {
                AppendToCrashLog("[UPDATER]: Installer download/verify failed");
            }
        }
    }

    if(!needsFullUpdate) {
        wchar_t pathBuf[MAX_PATH];
        GetModuleFileNameW(nullptr, pathBuf, MAX_PATH);
        std::wstring exeDir = pathBuf;
        size_t pos = exeDir.find_last_of(L"\\/");
        if(pos != std::wstring::npos) exeDir.erase(pos);

        for(const auto& key : partialUpdateKeys) {
            if(!files.contains(key)) continue;
            std::string url              = files[key]["url"].get<std::string>();
            std::string expectedChecksum = files[key]["checksum"].get<std::string>();
            std::filesystem::path localFile = std::filesystem::path(exeDir) /
                                              std::wstring(key.begin(), key.end());
            if(std::filesystem::exists(localFile) &&
               FileChecksum(localFile) == expectedChecksum) continue;

            if(DownloadFile(url, localFile) &&
               FileChecksum(localFile) == expectedChecksum) {
                if(key == "server.js") { StopNodeServer(); StartNodeServer(); }
            } else {
                AppendToCrashLog("[UPDATER]: Partial update failed for " + key);
            }
        }
    }
    std::cout << "[UPDATER]: Done.\n";
}

void RunInstallerAndExit()
{
    if(g_installerPath.empty()) {
        AppendToCrashLog("[UPDATER]: Installer path not set.");
        return;
    }
    wchar_t exeBuf[MAX_PATH];
    GetModuleFileNameW(nullptr, exeBuf, MAX_PATH);
    std::wstring dir(exeBuf);
    size_t pos = dir.find_last_of(L"\\/");
    if(pos != std::wstring::npos) dir.erase(pos);

    std::wstring args = L"/overrideInstallDir=\"" + dir + L"\"";
    HINSTANCE res = ShellExecuteW(nullptr, L"open", g_installerPath.c_str(), args.c_str(), nullptr, SW_HIDE);
    if((INT_PTR)res <= 32)
        AppendToCrashLog(L"[UPDATER]: ShellExecute failed for installer.");

    PostQuitMessage(0);
    exit(0);
}
