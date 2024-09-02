#include <iostream>
#include <string>
#include <curl/curl.h>
#include <fstream>
#include <chrono>
#include <sstream>
#include <nlohmann/json.hpp>
#include <iomanip>
#include <thread>
#include <chrono>
#include <regex>
#include <iostream>
#include <filesystem>

using json = nlohmann::json;
namespace fs = std::filesystem;



// URL encode a string
std::string urlEncode(const std::string &value) {
    CURL *curl = curl_easy_init();
    if (curl) {
        char *output = curl_easy_escape(curl, value.c_str(), value.length());
        std::string encodedStr = output;
        curl_free(output);
        curl_easy_cleanup(curl);
        return encodedStr;
    }
    return value;
}

// Function to send HTTP POST request and return the response
std::string postRequest(const std::string &url, const std::string &jsonData) {
    CURL *curl;
    CURLcode res;
    std::string response;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                         +[](char *ptr, size_t size, size_t nmemb, std::string *data) -> size_t {
                             data->append(ptr, size *nmemb);
                             return size *nmemb;
                         });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "POST request failed: " << curl_easy_strerror(res) << std::endl;
        }
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

    return response;
}

// Function to send HTTP GET request and return the response
std::string getRequest(const std::string &url) {
    CURL *curl;
    CURLcode res;
    std::string response;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                         +[](char *ptr, size_t size, size_t nmemb, std::string *data) -> size_t {
                             data->append(ptr, size *nmemb);
                             return size *nmemb;
                         });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "GET request failed: " << curl_easy_strerror(res) << std::endl;
        }
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

    return response;
}

// Function to read file data for upload
size_t read_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    std::ifstream *stream = (std::ifstream *)userdata;
    stream->read((char *)ptr, size *nmemb);
    return stream->gcount();
}

// Function to upload a file as an artifact using PUT request
bool uploadArtifact(const std::string &url, const std::string &filePath) {
    CURL *curl;
    CURLcode res;
    bool success = false;
    std::ifstream file(filePath, std::ios::in | std::ios::binary);
    if (!file) {
        std::cerr << "Could not open file: " << filePath << std::endl;
        return false;
    }

    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
        curl_easy_setopt(curl, CURLOPT_READDATA, &file);
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)fileSize);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                         +[](char *ptr, size_t size, size_t nmemb, std::string *data) -> size_t {
                             data->append(ptr, size *nmemb);
                             return size *nmemb;
                         });

        std::string response;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::cerr << "File upload failed: " << curl_easy_strerror(res) << std::endl;
        } else {
            success = true;
        }

        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

    return success;
}

// Function to create or get experiment ID
std::string createExperiment(const std::string &mlflow_addr, const std::string &experiment_name) {
    std::string create_exp_url = mlflow_addr + "/api/2.0/mlflow/experiments/create";
    json createExp;
    createExp["name"] = experiment_name;

    std::string createExpData = createExp.dump();
    std::string response = postRequest(create_exp_url, createExpData);

    json jsonResponse;
    try {
        jsonResponse = json::parse(response);
    } catch (const json::parse_error &e) {
        std::cerr << "Failed to parse JSON response: " << e.what() << std::endl;
        std::cerr << "Response was: " << response << std::endl;
        return "";
    }

    if (jsonResponse.contains("experiment_id")) {
        std::string experiment_id = jsonResponse["experiment_id"].get<std::string>();
        std::cout << "Created Experiment. Experiment ID: " << experiment_id << std::endl;
        return experiment_id;
    }

    std::cerr << "Failed to create experiment." << std::endl;
    std::cerr << "Response was: " << response << std::endl;
    return "";
}

// Function to confirm experiment creation by name
std::string getExperimentIdByName(const std::string &mlflow_addr, const std::string &experiment_name) {
    std::string get_exp_url = mlflow_addr + "/api/2.0/mlflow/experiments/get-by-name?experiment_name=" + urlEncode(experiment_name);
    std::string response = getRequest(get_exp_url);

    json jsonResponse;
    try {
        jsonResponse = json::parse(response);
    } catch (const json::parse_error &e) {
        std::cerr << "Failed to parse JSON response: " << e.what() << std::endl;
        return "";
    }

    if (jsonResponse.contains("experiment") && jsonResponse["experiment"].contains("experiment_id")) {
        return jsonResponse["experiment"]["experiment_id"].get<std::string>();
    }

    return "";
}

// Function to restore a deleted experiment
bool restoreExperiment(const std::string &mlflow_addr, const std::string &experiment_id) {
    std::string restore_exp_url = mlflow_addr + "/api/2.0/mlflow/experiments/restore";
    json restoreExp;
    restoreExp["experiment_id"] = experiment_id;

    std::string restoreExpData = restoreExp.dump();
    std::string response = postRequest(restore_exp_url, restoreExpData);

    // std::cout << "Restore request response: " << response << std::endl; // Add log

    json jsonResponse;
    try {
        jsonResponse = json::parse(response);
    } catch (const json::parse_error &e) {
        std::cerr << "Failed to parse JSON response: " << e.what() << std::endl;
        std::cerr << "Response was: " << response << std::endl;
        return false;
    }

    if (jsonResponse.contains("experiment_id") && jsonResponse["experiment_id"] == experiment_id) {
        // std::cout << "Restored Experiment. Experiment ID: " << experiment_id << std::endl;
        return true;
    }

    std::cerr << "Failed to restore experiment." << std::endl;
    std::cerr << "Response was: " << response << std::endl;
    return false;
}

// Function to start a new run and upload an artifact
bool uploadToMlflow(const std::string& mlflow_addr, const std::string& experiment_id,
                               const std::string& run_name, const std::string& file_path, std::string& file_url) {
    auto currentTime = std::chrono::system_clock::now();
    auto epoch = currentTime.time_since_epoch();
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();

    std::string start_run_url = mlflow_addr + "/api/2.0/mlflow/runs/create";
    json newRun;
    newRun["experiment_id"] = experiment_id;
    newRun["start_time"] = static_cast<json::number_integer_t>(milliseconds);
    newRun["run_name"] = run_name;  // Set the run name here
    newRun["tags"]["mlflow.runName"] = run_name;

    std::string newRunData = newRun.dump();
    std::string response = postRequest(start_run_url, newRunData);

    json jsonResponse;
    try {
        jsonResponse = json::parse(response);
    } catch (const json::parse_error &e) {
        std::cerr << "Failed to parse JSON response: " << e.what() << std::endl;
        std::cerr << "Response was: " << response << std::endl;
        return false;
    }

    std::string run_id;
    if (jsonResponse.contains("run") && jsonResponse["run"].contains("info") &&
        jsonResponse["run"]["info"].contains("run_id")) {
        run_id = jsonResponse["run"]["info"]["run_id"].get<std::string>();
    } else {
        std::cerr << "Error creating run." << std::endl;
        std::cerr << "Response was: " << response << std::endl;
        return false;
    }

    std::string csv_name = file_path.substr(file_path.find_last_of("/\\") + 1);
    std::string log_artifact_url = mlflow_addr + "/api/2.0/mlflow-artifacts/artifacts/" + experiment_id + "/" + run_id + "/artifacts/" + csv_name;

    if (!uploadArtifact(log_artifact_url, file_path)) {
        std::cerr << "Error uploading artifact." << std::endl;
        return false;
    }

    file_url = mlflow_addr + "/get-artifact?path=" + csv_name + "&run_uuid=" + run_id;
    // std::cout << "Artifact URL: " << file_url << std::endl;
    return true;
}

// Function to check and restore an experiment if it is in deleted state
std::string checkAndRestoreExperiment(const std::string &mlflow_addr, const std::string &experiment_name) {
    std::string experiment_id = getExperimentIdByName(mlflow_addr, experiment_name);
    if (!experiment_id.empty()) {
        // Check the state of the experiment
        std::string get_exp_url = mlflow_addr + "/api/2.0/mlflow/experiments/get?experiment_id=" + experiment_id;
        std::string response = getRequest(get_exp_url);

        json jsonResponse;
        try {
            jsonResponse = json::parse(response);
        } catch (const json::parse_error &e) {
            std::cerr << "Failed to parse JSON response: " << e.what() << std::endl;
            return "";
        }

        if (jsonResponse.contains("experiment") && jsonResponse["experiment"].contains("lifecycle_stage") &&
            jsonResponse["experiment"]["lifecycle_stage"] == "deleted") {
            // Restore the experiment if it's deleted
            if (!restoreExperiment(mlflow_addr, experiment_id)) {
                return "";
            }

            // Wait for a moment to ensure the restoration is processed
            std::this_thread::sleep_for(std::chrono::seconds(5)); // Increase the wait time to 5 seconds

            // Retry to get the experiment ID
            experiment_id = getExperimentIdByName(mlflow_addr, experiment_name);
            int retries = 5; // Retry up to 5 times
            while (experiment_id.empty() && retries > 0) {
                std::this_thread::sleep_for(std::chrono::seconds(2));
                experiment_id = getExperimentIdByName(mlflow_addr, experiment_name);
                retries--;
            }

            if (experiment_id.empty()) {
                std::cerr << "Failed to confirm experiment restoration." << std::endl;
                return "";
            }
        }

        return experiment_id;
    }

    experiment_id = createExperiment(mlflow_addr, experiment_name);

    // Wait and confirm experiment creation
    if (!experiment_id.empty()) {
        std::this_thread::sleep_for(std::chrono::seconds(5)); // Increase the wait time to 5 seconds

        // Retry to get the experiment ID
        experiment_id = getExperimentIdByName(mlflow_addr, experiment_name);
        int retries = 5; // Retry up to 5 times
        while (experiment_id.empty() && retries > 0) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            experiment_id = getExperimentIdByName(mlflow_addr, experiment_name);
            retries--;
        }

        if (experiment_id.empty()) {
            std::cerr << "Failed to confirm experiment creation." << std::endl;
        }
    }

    return experiment_id;
}

// Callback function to write data to a file
size_t writeData(void *ptr, size_t size, size_t nmemb, std::ofstream *stream) {
    stream->write(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

// Callback function to extract headers
size_t headerCallback(char *buffer, size_t size, size_t nitems, std::string *headers) {
    size_t totalSize = size * nitems;
    headers->append(buffer, totalSize);
    return totalSize;
}

// 存储函数
void vecToFile(const std::vector<std::string>& data, const std::string& filePath) {
    std::ofstream outFile(filePath);
    if (outFile.is_open()) {
        for (const auto& str : data) {
            outFile << str << "\n";
        }
        outFile.close();
    } else {
        std::cerr << "Could not open file for writing." << std::endl;
    }
}

void strToFile(const std::string& data, const std::string& filePath) {
    std::filesystem::path fs_path(data);
    std::string data_name = fs_path.filename().string();
    std::fstream file(filePath, std::ios::in | std::ios::out | std::ios::app);
    std::ofstream outFile(filePath, std::ios::app);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (line == data_name) {
                file.close();
                return;
            }
        }
        file.clear(); 
        file.seekp(0, std::ios::end); 
        file << data_name << std::endl; 
        file.close(); 
    
    }else {
        throw std::runtime_error("Could not open file for reading and writing.");
    }
}

// 读取函数
std::vector<std::string> vecFromFile(const std::string& filePath) {
    std::vector<std::string> data;
    std::ifstream inFile(filePath);
    if (inFile.is_open()) {
        std::string line;
        while (std::getline(inFile, line)) {
            data.push_back(line);
        }
        inFile.close();
    } else {
        std::cerr << "Could not open file for reading." << std::endl;
    }
    return data;
}


// Function to download a file from MLflow and save it with the original filename
bool downloadFromMlflow(const std::string& file_url, const std::string& out_path, const std::string& out_name = "") {
    CURL *curl;
    CURLcode res;
    std::string headers;
    std::string fileName;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, file_url.c_str());
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeData);
        
        std::ofstream file;
        // Dummy write function to capture headers first
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);
        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::cerr << "File download failed: " << curl_easy_strerror(res) << std::endl;
            return false;
        }

        // Extract filename from headers
        std::regex pattern(R"(filename=([^;\r\n]+))");
        std::smatch match;
        
        if (std::regex_search(headers, match, pattern)) {
            fileName = match[1];
        } else {
            std::cerr << "Filename not found in headers" << std::endl;
            return false;
        }

        // Reopen file with the correct filename
        if(out_name != ""){
            fileName = out_name;
        }
        if(!out_path.empty()) {
            fileName = out_path + "/" + fileName;
        }
        file.open(fileName, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Could not open file for writing: " << fileName << std::endl;
            return false;
        }

        // Download the file again with correct write callback
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "File download failed: " << curl_easy_strerror(res) << std::endl;
            file.close();
            return false;
        }
        file.close();

        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

    return true;
}

std::vector<std::pair<std::string, std::string>> getRunsForExperiment(const std::string &mlflow_addr, const std::string &experiment_id) {
    std::string url = mlflow_addr + "/api/2.0/mlflow/runs/search";
    std::string postData = "{\"experiment_ids\": [\"" + experiment_id + "\"]}";
    std::string response = postRequest(url, postData);

    json jsonResponse;
    try {
        jsonResponse = json::parse(response);
    } catch (const json::parse_error &e) {
        std::cerr << "Failed to parse JSON response: " << e.what() << std::endl;
        return {};
    }

    std::vector<std::pair<std::string, std::string>> runs_data;
    if (jsonResponse.contains("runs")) {
        for (const auto &run : jsonResponse["runs"]) {
            if (run.contains("data") && run["data"].contains("tags")) {
                for (const auto &tag : run["data"]["tags"]) {
                    if (tag.contains("key") && tag["key"] == "mlflow.runName") {
                        std::string run_id = run["info"]["run_id"].get<std::string>();
                        std::string value = tag["value"].get<std::string>();
                        runs_data.push_back({run_id, value});
                        break; // Assuming each run has only one mlflow.runName tag
                    }
                }
            }
        }
    }

    return runs_data;
}

bool downloadMlflowFileByName(const std::string& mlflow_addr, const std::string& exp_name, 
                              const std::string& target_file_name, const std::string& out_path, const std::string& out_name){
    std::string experiment_id = checkAndRestoreExperiment(mlflow_addr, exp_name);
    if (experiment_id.empty()) {
        std::cerr << "Failed to restore or create experiment." << std::endl;
        return false;
    }

    try {
        auto runs_data = getRunsForExperiment(mlflow_addr, experiment_id);
        for (const auto &run_data : runs_data) {
            std::string file_url = mlflow_addr + "/get-artifact?path=" + run_data.second + "&run_uuid=" + run_data.first;
            std::string file_name = run_data.second; // 获取文件名
            if (file_name == target_file_name) { // 如果文件名符合目标文件名
            
                if (downloadFromMlflow(file_url, out_path, out_name)) {
                    return true;
                } else {
                    std::cerr << "Failed to download: " << file_name << std::endl;
                }
            }
        }
        std::cout << "Not find " << target_file_name << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }

    return true;
}

//下载指定实验的全部文件
bool downloadMlflowFiles(const std::string& mlflow_addr, const std::string& exp_name, const std::string& out_path){
    std::string experiment_id = checkAndRestoreExperiment(mlflow_addr, exp_name);
    if (experiment_id.empty()) {
        std::cerr << "Failed to restore or create experiment." << std::endl;
        return false;
    }
    try {
        auto runs_data = getRunsForExperiment(mlflow_addr, experiment_id);
        // std::cout << "Run data for experiment " << experiment_id << ":\n";
        for (const auto &run_data : runs_data) {
            std::string file_url = mlflow_addr + "/get-artifact?path=" + run_data.second + "&run_uuid=" + run_data.first;
            if (!downloadFromMlflow(file_url, out_path)) {
                std::cerr << "Failed to download: " << run_data.second << std::endl;
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }
    return true;
}

//获取指定实验的所有文件名数据
std::vector<std::string> get_experiment_filenames(const std::string& mlflow_addr, const std::string& exp_name) {
    std::vector<std::string> file_names;

    std::string experiment_id = checkAndRestoreExperiment(mlflow_addr, exp_name);
    if (experiment_id.empty()) {
        std::cerr << "Failed to restore or create experiment." << std::endl;
        return file_names;
    }

    try {
        auto runs_data = getRunsForExperiment(mlflow_addr, experiment_id);
        for (const auto& run_data : runs_data) {
            file_names.push_back(run_data.second);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        file_names.clear();
    }

    return file_names;
}

//上传指定目录全部文件
bool uploadMlflowFiles(const std::string& mlflow_addr, const std::string& folder, std::string exp_name){
    if(exp_name.empty()){
        std::filesystem::path fs_path(folder);
        exp_name = fs_path.filename().string();
    }
    std::string exp_id = checkAndRestoreExperiment(mlflow_addr, exp_name);
    for (const auto & entry : fs::directory_iterator(folder)) {
        std::string file_path = fs::absolute(entry.path());
        std::string run_name = entry.path().filename();

        if (exp_id.empty()) {
            std::cerr << "Failed to restore or create experiment." << std::endl;
            return 1;
        }
        std::string file_url;
        if (uploadToMlflow(mlflow_addr, exp_id, run_name, file_path, file_url)) {
            std::cout << "Run and artifact upload successful." << std::endl;
        } else {
            std::cerr << "Run and artifact upload failed." << std::endl;
            return 1;
        }
    }
    return true;
}

// int main() {
//     std::string mlflow_addr = "http://192.168.50.10:8999";
//     std::string exp_name = "mpc-pir";
//     // std::string exp_id = checkAndRestoreExperiment(mlflow_addr, exp_name);
//     // downloadMlflowFiles(mlflow_addr, exp_name, "tmp_data1");
//     exp_name = "tmp_data2";
//     uploadMlflowFiles(mlflow_addr, exp_name, exp_name);

//     return 0;
// }

