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

using json = nlohmann::json;

namespace mpc {
namespace pir {


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

    std::cout << "Restore request response: " << response << std::endl; // Add log

    json jsonResponse;
    try {
        jsonResponse = json::parse(response);
    } catch (const json::parse_error &e) {
        std::cerr << "Failed to parse JSON response: " << e.what() << std::endl;
        std::cerr << "Response was: " << response << std::endl;
        return false;
    }

    if (jsonResponse.contains("experiment_id") && jsonResponse["experiment_id"] == experiment_id) {
        std::cout << "Restored Experiment. Experiment ID: " << experiment_id << std::endl;
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
    std::cout << "Artifact URL: " << file_url << std::endl;
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


// int main() {
//     std::string mlflow_addr = "http://192.168.50.10:8999";
//     std::string experiment_name = "mpc-pir6";
//     std::string run_name = "task_1";
//     std::string csv_path = "/root/lk/tee-sql-service/test/data/sql1.csv";

//     std::string experiment_id = checkAndRestoreExperiment(mlflow_addr, experiment_name);

//     if (experiment_id.empty()) {
//         std::cerr << "Failed to restore or create experiment." << std::endl;
//         return 1;
//     }
//     std::string file_url;
//     if (uploadToMlflow(mlflow_addr, experiment_id, run_name, csv_path, file_url)) {
//         std::cout << "Run and artifact upload successful." << std::endl;
//     } else {
//         std::cerr << "Run and artifact upload failed." << std::endl;
//         return 1;
//     }

//     return 0;
// }


}
}