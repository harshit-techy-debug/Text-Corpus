#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <regex>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
    #define CLOSE_SOCKET closesocket
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <cstring>
    #define CLOSE_SOCKET close
    #define SOCKET int
    #define INVALID_SOCKET -1
#endif

using namespace std;

// ==========================================
// Helper Functions
// ==========================================
bool hasParam(const string& query, const string& param) {
    return query.find(param + "=1") != string::npos;
}

string getParam(const string& query, const string& param, string defaultVal) {
    size_t pos = query.find(param + "=");
    if (pos == string::npos) return defaultVal;
    size_t end = query.find("&", pos);
    if (end == string::npos) end = query.length();
    return query.substr(pos + param.length() + 1, end - pos - param.length() - 1);
}

string escapeJson(const string& s) {
    string res;
    for (char c : s) {
        if (c == '"') res += "\\\"";
        else if (c == '\\') res += "\\\\";
        else if (c == '\n') res += "\\n";
        else if (c == '\r') res += "\\r";
        else if (c == '\t') res += "\\t";
        else res += c;
    }
    return res;
}

// ==========================================
// NLP Common Dictionaries
// ==========================================
const unordered_set<string> STOPWORDS = {
    "a", "about", "above", "after", "again", "against", "all", "am", "an", "and", "any", "are", "as", "at", 
    "be", "because", "been", "before", "being", "below", "between", "both", "but", "by", "can", "could", 
    "did", "do", "does", "doing", "down", "during", "each", "few", "for", "from", "further", "had", "has", 
    "have", "having", "he", "her", "here", "hers", "him", "his", "how", "i", "if", "in", "into", "is", "it", 
    "its", "just", "me", "more", "most", "my", "no", "nor", "not", "now", "of", "off", "on", "once", "only", 
    "or", "other", "our", "out", "over", "own", "same", "she", "should", "so", "some", "such", "than", "that", 
    "the", "their", "them", "then", "there", "these", "they", "this", "those", "through", "to", "too", "under", 
    "until", "up", "us", "very", "was", "we", "were", "what", "when", "where", "which", "while", "who", "whom", 
    "why", "will", "with", "would", "you", "your", "also", "may", "many", "use", "used", "using", "often"
};

// ==========================================
// Sentiment Analysis Dictionaries
// ==========================================
const unordered_set<string> POS_WORDS = {
    "love", "great", "awesome", "good", "happy", "excellent", "amazing", "fast", "easy", 
    "best", "wonderful", "fantastic", "joy", "positive", "nice", "perfect", "beautiful", 
    "enjoy", "helpful", "brilliant", "superb", "glad", "delight", "impressive"
};

const unordered_set<string> NEG_WORDS = {
    "hate", "bad", "terrible", "sad", "angry", "slow", "confusing", "hard", "worst", 
    "awful", "horrible", "negative", "poor", "disappointing", "annoying", "wrong", 
    "fail", "broken", "useless", "waste", "pain", "difficult", "problem", "issue"
};

const unordered_set<string> JOY_WORDS = {"happy", "joy", "love", "excellent", "great", "awesome", "smile", "delight", "glad"};
const unordered_set<string> SAD_WORDS = {"sad", "unhappy", "depressed", "cry", "gloomy", "heartbroken", "disappointing", "poor"};
const unordered_set<string> ANGER_WORDS = {"angry", "mad", "hate", "furious", "rage", "annoyed", "terrible", "worst"};
const unordered_set<string> FEAR_WORDS = {"scared", "fear", "afraid", "terrified", "anxious", "worried", "confusing"};

// ==========================================
// Core Logic Functions
// ==========================================
string processSentiment(const string& text) {
    string lowerText = text;
    transform(lowerText.begin(), lowerText.end(), lowerText.begin(), [](unsigned char c){ return tolower(c); });

    // Tokenize
    vector<string> words;
    string word;
    for (char c : lowerText) {
        if (isalnum(c) || c == '\'') {
            word += c;
        } else {
            if (!word.empty()) { words.push_back(word); word = ""; }
        }
    }
    if (!word.empty()) words.push_back(word);

    int totalWords = words.size();
    if (totalWords == 0) totalWords = 1; 

    int posCount = 0, negCount = 0;
    int joyCount = 0, sadCount = 0, angerCount = 0, fearCount = 0;
    bool negateNext = false;

    for (const string& w : words) {
        // Negation Handling
        if (w == "not" || w == "no" || w == "never" || w == "cannot" || w == "cant" || w == "dont") {
            negateNext = true;
            continue;
        }

        bool isPos = POS_WORDS.count(w);
        bool isNeg = NEG_WORDS.count(w);

        if (isPos) {
            if (negateNext) negCount++;
            else posCount++;
            negateNext = false;
        } 
        else if (isNeg) {
            if (negateNext) posCount++;
            else negCount++;
            negateNext = false;
        } 
        else {
            negateNext = false; 
        }
        
        // Emotions Count
        if (JOY_WORDS.count(w)) joyCount++;
        if (SAD_WORDS.count(w)) sadCount++;
        if (ANGER_WORDS.count(w)) angerCount++;
        if (FEAR_WORDS.count(w)) fearCount++;
    }

    int totalSentimentWords = posCount + negCount;
    int posScore = 0, negScore = 0, neuScore = 0;
    
    if (totalSentimentWords == 0) {
        neuScore = 100;
    } else {
        posScore = (posCount * 100) / totalSentimentWords;
        negScore = (negCount * 100) / totalSentimentWords;
        neuScore = 100 - posScore - negScore;
    }

    string label = "Neutral Sentiment";
    if (posScore > negScore) label = "Positive Sentiment";
    else if (negScore > posScore) label = "Negative Sentiment";

    double compound = 0.0;
    if (totalSentimentWords > 0) {
        compound = (double)(posCount - negCount) / totalSentimentWords;
    }

    int totalEmotions = joyCount + sadCount + angerCount + fearCount;
    int joyPct = 0, sadPct = 0, angerPct = 0, fearPct = 0;
    
    if (totalEmotions > 0) {
        joyPct = (joyCount * 100) / totalEmotions;
        sadPct = (sadCount * 100) / totalEmotions;
        angerPct = (angerCount * 100) / totalEmotions;
        fearPct = (fearCount * 100) / totalEmotions;
    } else {
        if (posScore > negScore) joyPct = 100;
        else if (negScore > posScore) sadPct = 100;
    }

    stringstream json;
    json << "{\n";
    json << "  \"total_words\": " << totalWords << ",\n";
    json << "  \"positive_score\": " << posScore << ",\n";
    json << "  \"negative_score\": " << negScore << ",\n";
    json << "  \"neutral_score\": " << neuScore << ",\n";
    json << "  \"compound\": " << fixed << setprecision(2) << compound << ",\n";
    json << "  \"overall_label\": \"" << label << "\",\n";
    json << "  \"emotions\": {\n";
    json << "    \"joy\": " << joyPct << ",\n";
    json << "    \"sadness\": " << sadPct << ",\n";
    json << "    \"anger\": " << angerPct << ",\n";
    json << "    \"fear\": " << fearPct << "\n";
    json << "  }\n";
    json << "}";
    
    return json.str();
}

// ==========================================
// Main Server
// ==========================================
int main() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 1;
#endif

    SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == INVALID_SOCKET) return 1;

    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);

    if (bind(server, (sockaddr*)&addr, sizeof(addr)) < 0) return 1;
    listen(server, 10);

    cout << "C++ Backend running at http://127.0.0.1:8080" << endl;
    cout << "Waiting for connections...\n" << endl;

    while (true) {
        SOCKET client = accept(server, nullptr, nullptr);
        if (client == INVALID_SOCKET) continue;

        char buffer[1048576] = {0}; 
        int n = recv(client, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) { CLOSE_SOCKET(client); continue; }
        
        string request(buffer, n);

        if (request.find("OPTIONS ") == 0) {
            string response = "HTTP/1.1 204 No Content\r\n"
                               "Access-Control-Allow-Origin: *\r\n"
                               "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
                               "Access-Control-Allow-Headers: Content-Type\r\n\r\n";
            send(client, response.c_str(), response.size(), 0);
            CLOSE_SOCKET(client);
            continue;
        }

        string pathAndQuery;
        size_t firstSpace = request.find(" ");
        size_t secondSpace = request.find(" ", firstSpace + 1);
        if (firstSpace != string::npos && secondSpace != string::npos) {
            pathAndQuery = request.substr(firstSpace + 1, secondSpace - firstSpace - 1);
        }

        string body;
        size_t headerEnd = request.find("\r\n\r\n");
        if (headerEnd != string::npos) body = request.substr(headerEnd + 4);

        string responseBody = "";
        string contentType = "application/json";

        // Sentiment API Route
        if (pathAndQuery.find("/api/sentiment") == 0) {
            cout << "--> Processing Sentiment Analysis..." << endl;
            responseBody = processSentiment(body);
            cout << "--> Sent Sentiment Response!\n" << endl;
        } else {
            responseBody = "{\"error\": \"API not found\"}";
        }

        string response = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: " + contentType + "\r\n"
                           "Access-Control-Allow-Origin: *\r\n"
                           "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
                           "Access-Control-Allow-Headers: Content-Type\r\n"
                           "Content-Length: " + to_string(responseBody.size()) + "\r\n"
                           "\r\n" + responseBody;

        send(client, response.c_str(), response.size(), 0);
        CLOSE_SOCKET(client);
    }

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}