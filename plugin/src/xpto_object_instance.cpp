#include "xpto_object_instance.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

#include "XPLMDataAccess.h"
#include "XPLMInstance.h"
#include "XPLMPlanes.h"
#include "XPLMScenery.h"
#include "XPLMUtilities.h"

namespace {

constexpr double kDegreesToRadians = 3.14159265358979323846 / 180.0;

struct ProxyDefinition {
    const char* name;
    const char* id;
    const char* objectPath;
    float baseWidth;
    float baseHeight;
    xpto::ProxyPosition defaultPlacement;
    xpto::ProxyRotation defaultRotation;
};

struct CalibrationRecord {
    bool exists = false;
    bool calibrated = false;
    xpto::ProxyPosition baselinePlacement;
    xpto::ProxyRotation baselineRotation;
    xpto::ProxyPosition proxyPlacement;
    xpto::ProxyRotation proxyRotation;
    xpto::ProxyPosition offsetPlacement;
    xpto::ProxyRotation offsetRotation;
    xpto::ProxySize size;
};

struct RuntimeProxy {
    ProxyDefinition definition;
    XPLMObjectRef object = nullptr;
    XPLMInstanceRef instance = nullptr;
    XPLMDrawInfo_t drawInfo = {};
    std::string runtimeObjectPath;
    xpto::ProxyPosition aircraftLocalPosition;
    xpto::ProxyPosition placement;
    xpto::ProxyRotation rotation;
    xpto::ProxySize size;
    float baseHeadingDegrees = 0.0f;
    bool initialized = false;
    bool calibrationLoaded = false;
    CalibrationRecord calibration;
};

RuntimeProxy g_proxies[] = {
    {{"GTN750 U1", "gtn750-u1", "Resources/plugins/XPTO/assets/xpto_gtn750_proxy.obj", 0.1322f, 0.1500f, {1.360000014f, 0.899999976f, -1.350000024f}, {}}},
    {{"GTN650 U2", "gtn650-u2", "Resources/plugins/XPTO/assets/xpto_gtn650_proxy.obj", 0.1322f, 0.0660f, {1.360000014f, 0.300000012f, -1.350000024f}, {}}},
};

int g_selectedProxyIndex = 0;
XPLMDataRef g_localX = nullptr;
XPLMDataRef g_localY = nullptr;
XPLMDataRef g_localZ = nullptr;
XPLMDataRef g_truePsi = nullptr;
bool g_lastExportSucceeded = false;
char g_lastExportPath[512] = {};
char g_calibrationPath[512] = {};
char g_aircraftFile[512] = {};
char g_aircraftPath[1024] = {};
char g_aircraftFolder[256] = {};
char g_profileId[64] = {};
bool g_aircraftIdentityLoaded = false;

int ProxyCount() {
    return static_cast<int>(sizeof(g_proxies) / sizeof(g_proxies[0]));
}

RuntimeProxy& SelectedProxy() {
    return g_proxies[g_selectedProxyIndex];
}

void Log(const char* message) {
    XPLMDebugString(message);
}

void Timestamp(char* output, int outputSize);


std::string SanitizeKeyPart(const char* value) {
    std::string output;
    for (const char* cursor = value; cursor != nullptr && *cursor != '\0'; ++cursor) {
        const char c = *cursor;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-') {
            output += c;
        } else {
            output += '_';
        }
    }
    if (output.empty()) {
        output = "unknown";
    }
    return output;
}

void EnsureAircraftIdentity() {
    if (g_aircraftIdentityLoaded) {
        return;
    }

    g_aircraftFile[0] = '\0';
    g_aircraftPath[0] = '\0';
    g_aircraftFolder[0] = '\0';
    g_profileId[0] = '\0';
    XPLMGetNthAircraftModel(0, g_aircraftFile, g_aircraftPath);

    std::filesystem::path path(g_aircraftPath);
    std::filesystem::path folder = path;
    if (path.has_extension()) {
        folder = path.parent_path();
    }
    std::snprintf(g_aircraftFolder, sizeof(g_aircraftFolder), "%s", folder.filename().string().c_str());
    g_aircraftIdentityLoaded = true;
}

std::string CalibrationKey(const RuntimeProxy& proxy) {
    EnsureAircraftIdentity();
    std::string key;
    key += SanitizeKeyPart(g_aircraftFolder);
    key += "|";
    key += SanitizeKeyPart(g_aircraftFile);
    key += "|";
    key += SanitizeKeyPart(g_profileId);
    key += "|";
    key += proxy.definition.id;
    return key;
}


std::filesystem::path XPlaneSystemPath() {
    char systemPath[1024] = {};
    XPLMGetSystemPath(systemPath);
    return std::filesystem::path(systemPath);
}

std::filesystem::path AbsolutePluginPath(const char* relativePath) {
    return XPlaneSystemPath() / std::filesystem::path(relativePath);
}


std::filesystem::path CalibrationPath() {
    return XPlaneSystemPath() / "Resources" / "plugins" / "XPTO" / "config" / "xpto-proxy-calibrations.json";
}

std::string ReadTextFile(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) {
        return {};
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

std::string JsonString(const char* value) {
    std::ostringstream out;
    out << '"';
    for (const char* cursor = value; cursor != nullptr && *cursor != '\0'; ++cursor) {
        if (*cursor == '"' || *cursor == '\\') {
            out << '\\' << *cursor;
        } else if (*cursor == '\n') {
            out << "\\n";
        } else if (*cursor == '\r') {
            out << "\\r";
        } else if (*cursor == '\t') {
            out << "\\t";
        } else {
            out << *cursor;
        }
    }
    out << '"';
    return out.str();
}

size_t FindMatchingBrace(const std::string& text, size_t openBrace) {
    int depth = 0;
    bool inString = false;
    bool escaped = false;
    for (size_t i = openBrace; i < text.size(); ++i) {
        const char c = text[i];
        if (inString) {
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                inString = false;
            }
            continue;
        }
        if (c == '"') {
            inString = true;
        } else if (c == '{') {
            ++depth;
        } else if (c == '}') {
            --depth;
            if (depth == 0) {
                return i;
            }
        }
    }
    return std::string::npos;
}

std::string FindObjectBlock(const std::string& text, const char* name) {
    std::string needle = "\"";
    needle += name;
    needle += "\"";
    const size_t namePos = text.find(needle);
    if (namePos == std::string::npos) {
        return {};
    }
    const size_t openBrace = text.find('{', namePos + needle.size());
    if (openBrace == std::string::npos) {
        return {};
    }
    const size_t closeBrace = FindMatchingBrace(text, openBrace);
    if (closeBrace == std::string::npos) {
        return {};
    }
    return text.substr(openBrace, closeBrace - openBrace + 1);
}

bool ExtractBool(const std::string& block, const char* field, bool fallback) {
    std::string needle = "\"";
    needle += field;
    needle += "\"";
    const size_t fieldPos = block.find(needle);
    if (fieldPos == std::string::npos) {
        return fallback;
    }
    const size_t colon = block.find(':', fieldPos + needle.size());
    if (colon == std::string::npos) {
        return fallback;
    }
    const size_t valueStart = block.find_first_not_of(" \t\r\n", colon + 1);
    if (valueStart == std::string::npos) {
        return fallback;
    }
    return block.compare(valueStart, 4, "true") == 0;
}

float ExtractFloat(const std::string& block, const char* field, float fallback) {
    std::string needle = "\"";
    needle += field;
    needle += "\"";
    const size_t fieldPos = block.find(needle);
    if (fieldPos == std::string::npos) {
        return fallback;
    }
    const size_t colon = block.find(':', fieldPos + needle.size());
    if (colon == std::string::npos) {
        return fallback;
    }
    const char* start = block.c_str() + colon + 1;
    char* end = nullptr;
    const double parsed = std::strtod(start, &end);
    return end == start ? fallback : static_cast<float>(parsed);
}

xpto::ProxyPosition ExtractPosition(const std::string& block, xpto::ProxyPosition fallback) {
    fallback.x = ExtractFloat(block, "x", fallback.x);
    fallback.y = ExtractFloat(block, "y", fallback.y);
    fallback.z = ExtractFloat(block, "z", fallback.z);
    return fallback;
}

xpto::ProxyRotation ExtractRotation(const std::string& block, xpto::ProxyRotation fallback) {
    fallback.pitch = ExtractFloat(block, "pitch", fallback.pitch);
    fallback.yaw = ExtractFloat(block, "yaw", fallback.yaw);
    fallback.roll = ExtractFloat(block, "roll", fallback.roll);
    return fallback;
}

xpto::ProxySize ExtractSize(const std::string& block, xpto::ProxySize fallback) {
    fallback.width = ExtractFloat(block, "width", fallback.width);
    fallback.height = ExtractFloat(block, "height", fallback.height);
    fallback.scaleX = ExtractFloat(block, "scale_x", fallback.scaleX);
    fallback.scaleY = ExtractFloat(block, "scale_y", fallback.scaleY);
    return fallback;
}

std::string RuntimeObjectRelativePath(const RuntimeProxy& proxy) {
    std::string path = "Resources/plugins/XPTO/exports/";
    path += proxy.definition.id;
    path += "-proxy-runtime.obj";
    return path;
}

void WriteQuadVertices(std::ofstream& out, float left, float top, float right, float bottom) {
    out << "VT " << left << " " << top << " 0.000 0.0 0.0 1.0 0.0 1.0\n";
    out << "VT " << right << " " << top << " 0.000 0.0 0.0 1.0 1.0 1.0\n";
    out << "VT " << right << " " << bottom << " 0.000 0.0 0.0 1.0 1.0 0.0\n";
    out << "VT " << left << " " << bottom << " 0.000 0.0 0.0 1.0 0.0 0.0\n";
}

bool WriteProxyObj(const std::filesystem::path& path, float width, float height) {
    try {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream out(path, std::ios::trunc);
        if (!out) {
            return false;
        }

        const float halfWidth = width * 0.5f;
        const float halfHeight = height * 0.5f;
        float border = width < height ? width * 0.06f : height * 0.06f;
        if (border < 0.002f) {
            border = 0.002f;
        }
        if (border > 0.01f) {
            border = 0.01f;
        }

        out << "A\n800\nOBJ\n\n";
        out << "POINT_COUNTS 20 0 0 30\n\n";
        out << "# XPTO runtime placement proxy. Dark fill, magenta border.\n";
        WriteQuadVertices(out, -halfWidth, halfHeight, halfWidth, -halfHeight);
        WriteQuadVertices(out, -halfWidth, halfHeight, -halfWidth + border, -halfHeight);
        WriteQuadVertices(out, halfWidth - border, halfHeight, halfWidth, -halfHeight);
        WriteQuadVertices(out, -halfWidth, halfHeight, halfWidth, halfHeight - border);
        WriteQuadVertices(out, -halfWidth, -halfHeight + border, halfWidth, -halfHeight);
        out << "\n";
        for (int i = 0; i < 5; ++i) {
            const int base = i * 4;
            out << "IDX " << base << "\n";
            out << "IDX " << base + 1 << "\n";
            out << "IDX " << base + 2 << "\n";
            out << "IDX " << base << "\n";
            out << "IDX " << base + 2 << "\n";
            out << "IDX " << base + 3 << "\n";
        }
        out << "\nATTR_no_cull\n";
        out << "ATTR_no_blend\n";
        out << "ATTR_diffuse_rgb 0.02 0.02 0.02\n";
        out << "TRIS 0 6\n";
        out << "ATTR_diffuse_rgb 1.00 0.00 1.00\n";
        out << "TRIS 6 24\n";
        out << "ATTR_diffuse_rgb 1.00 1.00 1.00\n";
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool WriteRuntimeProxyObj(RuntimeProxy& proxy) {
    proxy.runtimeObjectPath = RuntimeObjectRelativePath(proxy);
    const std::filesystem::path absolutePath = AbsolutePluginPath(proxy.runtimeObjectPath.c_str());
    const bool wrote = WriteProxyObj(absolutePath, proxy.size.width, proxy.size.height);

    char buffer[512] = {};
    std::snprintf(
        buffer,
        sizeof(buffer),
        "XPTO: %s runtime proxy OBJ %s width=%.4f height=%.4f path=%s\n",
        wrote ? "wrote" : "failed to write",
        proxy.definition.name,
        proxy.size.width,
        proxy.size.height,
        proxy.runtimeObjectPath.c_str());
    Log(buffer);
    return wrote;
}

void LogState(const RuntimeProxy& proxy, const char* label) {
    char buffer[768] = {};
    std::snprintf(
        buffer,
        sizeof(buffer),
        "XPTO: %s proxy=%s acf_candidate x=%.3f y=%.3f z=%.3f rot roll=%.2f pitch=%.2f yaw=%.2f size width=%.4f height=%.4f scale_x=%.3f scale_y=%.3f final local x=%.2f y=%.2f z=%.2f pitch=%.2f heading=%.2f roll=%.2f\n",
        label,
        proxy.definition.name,
        proxy.placement.x,
        proxy.placement.y,
        proxy.placement.z,
        proxy.rotation.roll,
        proxy.rotation.pitch,
        proxy.rotation.yaw,
        proxy.size.width,
        proxy.size.height,
        proxy.size.scaleX,
        proxy.size.scaleY,
        proxy.drawInfo.x,
        proxy.drawInfo.y,
        proxy.drawInfo.z,
        proxy.drawInfo.pitch,
        proxy.drawInfo.heading,
        proxy.drawInfo.roll);
    Log(buffer);
}

void FindPositionDataRefs() {
    if (g_localX != nullptr && g_localY != nullptr && g_localZ != nullptr && g_truePsi != nullptr) {
        return;
    }

    g_localX = XPLMFindDataRef("sim/flightmodel/position/local_x");
    g_localY = XPLMFindDataRef("sim/flightmodel/position/local_y");
    g_localZ = XPLMFindDataRef("sim/flightmodel/position/local_z");
    g_truePsi = XPLMFindDataRef("sim/flightmodel/position/true_psi");
}

void ResetProxyControlState(RuntimeProxy& proxy) {
    proxy.placement = proxy.definition.defaultPlacement;
    proxy.rotation = proxy.definition.defaultRotation;
    proxy.size.width = proxy.definition.baseWidth;
    proxy.size.height = proxy.definition.baseHeight;
    proxy.size.scaleX = 1.0f;
    proxy.size.scaleY = 1.0f;
}

void RecomputeDrawInfoFromControlState(RuntimeProxy& proxy) {
    const double headingRadians = proxy.baseHeadingDegrees * kDegreesToRadians;
    const double sinHeading = std::sin(headingRadians);
    const double cosHeading = std::cos(headingRadians);

    proxy.drawInfo.structSize = sizeof(proxy.drawInfo);
    proxy.drawInfo.x = static_cast<float>(proxy.aircraftLocalPosition.x + sinHeading * proxy.placement.z - cosHeading * proxy.placement.x);
    proxy.drawInfo.y = proxy.aircraftLocalPosition.y + proxy.placement.y;
    proxy.drawInfo.z = static_cast<float>(proxy.aircraftLocalPosition.z - cosHeading * proxy.placement.z - sinHeading * proxy.placement.x);
    proxy.drawInfo.pitch = proxy.rotation.pitch;
    proxy.drawInfo.heading = proxy.baseHeadingDegrees + proxy.rotation.yaw;
    proxy.drawInfo.roll = proxy.rotation.roll;
}

void ApplySavedCalibrationStart(RuntimeProxy& proxy) {
    if (proxy.calibration.calibrated) {
        proxy.placement = proxy.calibration.proxyPlacement;
        proxy.rotation = proxy.calibration.proxyRotation;
        proxy.size = proxy.calibration.size;
        proxy.size.scaleX = proxy.size.width / proxy.definition.baseWidth;
        proxy.size.scaleY = proxy.size.height / proxy.definition.baseHeight;
        if (proxy.size.width != proxy.definition.baseWidth || proxy.size.height != proxy.definition.baseHeight) {
            WriteRuntimeProxyObj(proxy);
        }
    } else if (proxy.calibration.exists) {
        proxy.placement = proxy.calibration.baselinePlacement;
        proxy.rotation = proxy.calibration.baselineRotation;
    }
}

void InitializeProxyNearAircraft(RuntimeProxy& proxy) {
    FindPositionDataRefs();

    proxy.drawInfo = {};
    ResetProxyControlState(proxy);
    ApplySavedCalibrationStart(proxy);
    proxy.baseHeadingDegrees = 0.0f;

    if (g_localX == nullptr || g_localY == nullptr || g_localZ == nullptr) {
        Log("XPTO: local position datarefs unavailable; rendering ACF candidate placement relative to local origin fallback.\n");
        proxy.aircraftLocalPosition = {};
        proxy.initialized = true;
        RecomputeDrawInfoFromControlState(proxy);
        LogState(proxy, "initialized fallback");
        return;
    }

    const double aircraftX = XPLMGetDatad(g_localX);
    const double aircraftY = XPLMGetDatad(g_localY);
    const double aircraftZ = XPLMGetDatad(g_localZ);
    const double aircraftHeadingDegrees = g_truePsi != nullptr ? XPLMGetDataf(g_truePsi) : 0.0;
    char aircraftBuffer[320] = {};
    std::snprintf(
        aircraftBuffer,
        sizeof(aircraftBuffer),
        "XPTO: aircraft local position for proxy=%s x=%.2f y=%.2f z=%.2f true_psi=%.1f\n",
        proxy.definition.name,
        aircraftX,
        aircraftY,
        aircraftZ,
        aircraftHeadingDegrees);
    Log(aircraftBuffer);

    proxy.aircraftLocalPosition.x = static_cast<float>(aircraftX);
    proxy.aircraftLocalPosition.y = static_cast<float>(aircraftY);
    proxy.aircraftLocalPosition.z = static_cast<float>(aircraftZ);
    proxy.baseHeadingDegrees = static_cast<float>(aircraftHeadingDegrees);
    proxy.initialized = true;

    Log("XPTO: proxy source placement is ACF candidate x/y/z/pitch/yaw/roll. Rendering converts from aircraft/body coordinates to X-Plane local coordinates.\n");
    RecomputeDrawInfoFromControlState(proxy);
    LogState(proxy, "initialized");
}

bool EnsureObjectLoaded(RuntimeProxy& proxy) {
    if (proxy.object != nullptr) {
        return true;
    }

    const char* objectPath = proxy.runtimeObjectPath.empty() ? proxy.definition.objectPath : proxy.runtimeObjectPath.c_str();
    char buffer[512] = {};
    std::snprintf(buffer, sizeof(buffer), "XPTO: loading proxy=%s OBJ path: %s\n", proxy.definition.name, objectPath);
    Log(buffer);
    proxy.object = XPLMLoadObject(objectPath);

    if (proxy.object == nullptr) {
        std::snprintf(buffer, sizeof(buffer), "XPTO: failed to load proxy=%s OBJ. Confirm asset is installed at %s\n", proxy.definition.name, objectPath);
        Log(buffer);
        return false;
    }

    std::snprintf(buffer, sizeof(buffer), "XPTO: proxy=%s OBJ loaded successfully.\n", proxy.definition.name);
    Log(buffer);
    return true;
}

bool EnsureInstanceCreated(RuntimeProxy& proxy) {
    if (proxy.instance != nullptr) {
        return true;
    }

    if (!proxy.initialized) {
        InitializeProxyNearAircraft(proxy);
    }

    if (!EnsureObjectLoaded(proxy)) {
        return false;
    }

    const char* datarefs[] = {nullptr};
    proxy.instance = XPLMCreateInstance(proxy.object, datarefs);

    if (proxy.instance == nullptr) {
        char buffer[256] = {};
        std::snprintf(buffer, sizeof(buffer), "XPTO: failed to create proxy=%s instance.\n", proxy.definition.name);
        Log(buffer);
        return false;
    }

    char buffer[256] = {};
    std::snprintf(buffer, sizeof(buffer), "XPTO: proxy=%s instance created.\n", proxy.definition.name);
    Log(buffer);
    return true;
}

void ApplyPosition(RuntimeProxy& proxy, const char* label) {
    if (proxy.instance == nullptr) {
        return;
    }

    RecomputeDrawInfoFromControlState(proxy);
    XPLMInstanceSetPosition(proxy.instance, &proxy.drawInfo, nullptr);
    LogState(proxy, label);
}

void ShowProxy(RuntimeProxy& proxy) {
    if (!EnsureInstanceCreated(proxy)) {
        return;
    }

    ApplyPosition(proxy, "shown");
}

void HideProxy(RuntimeProxy& proxy) {
    if (proxy.instance == nullptr) {
        char buffer[256] = {};
        std::snprintf(buffer, sizeof(buffer), "XPTO: hide requested for proxy=%s, but no instance exists.\n", proxy.definition.name);
        Log(buffer);
        return;
    }

    XPLMDestroyInstance(proxy.instance);
    proxy.instance = nullptr;

    char buffer[256] = {};
    std::snprintf(buffer, sizeof(buffer), "XPTO: proxy=%s instance destroyed/hidden.\n", proxy.definition.name);
    Log(buffer);
}


void LoadCalibration(RuntimeProxy& proxy) {
    EnsureAircraftIdentity();
    proxy.calibrationLoaded = true;
    proxy.calibration = {};
    proxy.calibration.baselinePlacement = proxy.definition.defaultPlacement;
    proxy.calibration.baselineRotation = proxy.definition.defaultRotation;
    proxy.calibration.proxyPlacement = proxy.definition.defaultPlacement;
    proxy.calibration.proxyRotation = proxy.definition.defaultRotation;
    proxy.calibration.size.width = proxy.definition.baseWidth;
    proxy.calibration.size.height = proxy.definition.baseHeight;
    proxy.calibration.size.scaleX = 1.0f;
    proxy.calibration.size.scaleY = 1.0f;

    const std::filesystem::path path = CalibrationPath();
    std::snprintf(g_calibrationPath, sizeof(g_calibrationPath), "%s", path.string().c_str());

    const std::string text = ReadTextFile(path);
    if (text.empty()) {
        return;
    }

    const std::string key = CalibrationKey(proxy);
    std::string keyNeedle = "\"" + key + "\"";
    const size_t keyPos = text.find(keyNeedle);
    if (keyPos == std::string::npos) {
        return;
    }
    const size_t openBrace = text.find('{', keyPos + keyNeedle.size());
    if (openBrace == std::string::npos) {
        return;
    }
    const size_t closeBrace = FindMatchingBrace(text, openBrace);
    if (closeBrace == std::string::npos) {
        return;
    }

    const std::string record = text.substr(openBrace, closeBrace - openBrace + 1);
    proxy.calibration.exists = true;
    proxy.calibration.calibrated = ExtractBool(record, "calibrated", false);

    const std::string baseline = FindObjectBlock(record, "acf_baseline_placement");
    if (!baseline.empty()) {
        proxy.calibration.baselinePlacement = ExtractPosition(baseline, proxy.calibration.baselinePlacement);
        proxy.calibration.baselineRotation = ExtractRotation(baseline, proxy.calibration.baselineRotation);
    }

    const std::string source = FindObjectBlock(record, "proxy_source_at_visual_match");
    if (!source.empty()) {
        proxy.calibration.proxyPlacement = ExtractPosition(source, proxy.calibration.proxyPlacement);
        proxy.calibration.proxyRotation = ExtractRotation(source, proxy.calibration.proxyRotation);
    }

    const std::string offset = FindObjectBlock(record, "calibration_offset");
    if (!offset.empty()) {
        proxy.calibration.offsetPlacement = ExtractPosition(offset, proxy.calibration.offsetPlacement);
        proxy.calibration.offsetRotation = ExtractRotation(offset, proxy.calibration.offsetRotation);
    }

    const std::string size = FindObjectBlock(record, "size");
    if (!size.empty()) {
        proxy.calibration.size = ExtractSize(size, proxy.calibration.size);
    }

    char buffer[512] = {};
    std::snprintf(
        buffer,
        sizeof(buffer),
        "XPTO: loaded calibration target=%s calibrated=%s offset x=%.3f y=%.3f z=%.3f path=%s\n",
        proxy.definition.name,
        proxy.calibration.calibrated ? "true" : "false",
        proxy.calibration.offsetPlacement.x,
        proxy.calibration.offsetPlacement.y,
        proxy.calibration.offsetPlacement.z,
        g_calibrationPath);
    Log(buffer);
}

void EnsureCalibrationLoaded(RuntimeProxy& proxy) {
    if (!proxy.calibrationLoaded) {
        LoadCalibration(proxy);
    }
}

void WritePositionJson(std::ofstream& out, const char* name, const xpto::ProxyPosition& position, const xpto::ProxyRotation& rotation, const char* suffix) {
    out << "    \"" << name << "\": {\n";
    out << "      \"x\": " << position.x << ",\n";
    out << "      \"y\": " << position.y << ",\n";
    out << "      \"z\": " << position.z << ",\n";
    out << "      \"pitch\": " << rotation.pitch << ",\n";
    out << "      \"yaw\": " << rotation.yaw << ",\n";
    out << "      \"roll\": " << rotation.roll << "\n";
    out << "    }" << suffix << "\n";
}

std::string CalibrationRecordJson(const RuntimeProxy& proxy, const char* timestamp) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(9);
    out << "    \"" << CalibrationKey(proxy) << "\": {\n";
    out << "      \"calibrated\": " << (proxy.calibration.calibrated ? "true" : "false") << ",\n";
    out << "      \"timestamp\": " << JsonString(timestamp) << ",\n";
    out << "      \"notes\": \"developer calibration record; safe to edit manually\",\n";
    out << "      \"aircraft\": {\n";
    out << "        \"acf_file\": " << JsonString(g_aircraftFile) << ",\n";
    out << "        \"aircraft_path\": " << JsonString(g_aircraftPath) << ",\n";
    out << "        \"aircraft_folder\": " << JsonString(g_aircraftFolder) << ",\n";
    out << "        \"profile_id\": " << JsonString(g_profileId) << "\n";
    out << "      },\n";
    out << "      \"target\": {\n";
    out << "        \"id\": " << JsonString(proxy.definition.id) << ",\n";
    out << "        \"name\": " << JsonString(proxy.definition.name) << "\n";
    out << "      },\n";
    out << "      \"acf_baseline_placement\": {\n";
    out << "        \"x\": " << proxy.calibration.baselinePlacement.x << ",\n";
    out << "        \"y\": " << proxy.calibration.baselinePlacement.y << ",\n";
    out << "        \"z\": " << proxy.calibration.baselinePlacement.z << ",\n";
    out << "        \"pitch\": " << proxy.calibration.baselineRotation.pitch << ",\n";
    out << "        \"yaw\": " << proxy.calibration.baselineRotation.yaw << ",\n";
    out << "        \"roll\": " << proxy.calibration.baselineRotation.roll << "\n";
    out << "      },\n";
    out << "      \"proxy_source_at_visual_match\": {\n";
    out << "        \"x\": " << proxy.calibration.proxyPlacement.x << ",\n";
    out << "        \"y\": " << proxy.calibration.proxyPlacement.y << ",\n";
    out << "        \"z\": " << proxy.calibration.proxyPlacement.z << ",\n";
    out << "        \"pitch\": " << proxy.calibration.proxyRotation.pitch << ",\n";
    out << "        \"yaw\": " << proxy.calibration.proxyRotation.yaw << ",\n";
    out << "        \"roll\": " << proxy.calibration.proxyRotation.roll << "\n";
    out << "      },\n";
    out << "      \"calibration_offset\": {\n";
    out << "        \"x\": " << proxy.calibration.offsetPlacement.x << ",\n";
    out << "        \"y\": " << proxy.calibration.offsetPlacement.y << ",\n";
    out << "        \"z\": " << proxy.calibration.offsetPlacement.z << ",\n";
    out << "        \"pitch\": " << proxy.calibration.offsetRotation.pitch << ",\n";
    out << "        \"yaw\": " << proxy.calibration.offsetRotation.yaw << ",\n";
    out << "        \"roll\": " << proxy.calibration.offsetRotation.roll << "\n";
    out << "      },\n";
    out << "      \"size\": {\n";
    out << "        \"width\": " << proxy.calibration.size.width << ",\n";
    out << "        \"height\": " << proxy.calibration.size.height << ",\n";
    out << "        \"scale_x\": " << proxy.calibration.size.scaleX << ",\n";
    out << "        \"scale_y\": " << proxy.calibration.size.scaleY << "\n";
    out << "      }\n";
    out << "    }";
    return out.str();
}

bool SaveCalibration(RuntimeProxy& proxy) {
    EnsureAircraftIdentity();
    char timestamp[64] = {};
    Timestamp(timestamp, sizeof(timestamp));

    const std::filesystem::path path = CalibrationPath();
    std::snprintf(g_calibrationPath, sizeof(g_calibrationPath), "%s", path.string().c_str());
    std::string text = ReadTextFile(path);
    const std::string record = CalibrationRecordJson(proxy, timestamp);
    const std::string key = CalibrationKey(proxy);
    const std::string keyNeedle = "\"" + key + "\"";

    try {
        std::filesystem::create_directories(path.parent_path());

        const size_t keyPos = text.find(keyNeedle);
        if (keyPos != std::string::npos) {
            const size_t lineStart = text.rfind("\n", keyPos);
            const size_t recordStart = lineStart == std::string::npos ? keyPos : lineStart + 1;
            const size_t openBrace = text.find('{', keyPos + keyNeedle.size());
            const size_t closeBrace = openBrace == std::string::npos ? std::string::npos : FindMatchingBrace(text, openBrace);
            if (closeBrace != std::string::npos) {
                size_t recordEnd = closeBrace + 1;
                const bool hadTrailingComma = recordEnd < text.size() && text[recordEnd] == ',';
                if (hadTrailingComma) {
                    ++recordEnd;
                }
                text.replace(recordStart, recordEnd - recordStart, hadTrailingComma ? record + "," : record);
            }
        } else if (!text.empty()) {
            size_t recordsPos = text.find("\"records\"");
            size_t recordsOpen = recordsPos == std::string::npos ? std::string::npos : text.find('{', recordsPos);
            size_t recordsClose = recordsOpen == std::string::npos ? std::string::npos : FindMatchingBrace(text, recordsOpen);
            if (recordsClose != std::string::npos) {
                const bool hasExistingRecord = text.find('"', recordsOpen + 1) < recordsClose;
                std::string insertion = hasExistingRecord ? ",\n" + record + "\n" : "\n" + record + "\n";
                text.insert(recordsClose, insertion);
            } else {
                text.clear();
            }
        }

        if (text.empty()) {
            text = "{\n  \"schema\": \"xpto-proxy-calibrations-v1\",\n  \"records\": {\n" + record + "\n  }\n}\n";
        }

        std::ofstream out(path, std::ios::trunc);
        if (!out) {
            return false;
        }
        out << text;
    } catch (const std::exception&) {
        return false;
    }

    char buffer[512] = {};
    std::snprintf(buffer, sizeof(buffer), "XPTO: saved calibration target=%s path=%s\n", proxy.definition.name, g_calibrationPath);
    Log(buffer);
    return true;
}

void DestroyProxy(RuntimeProxy& proxy) {
    if (proxy.instance != nullptr) {
        XPLMDestroyInstance(proxy.instance);
        proxy.instance = nullptr;
        char buffer[256] = {};
        std::snprintf(buffer, sizeof(buffer), "XPTO: proxy=%s instance destroyed during plugin cleanup.\n", proxy.definition.name);
        Log(buffer);
    }

    if (proxy.object != nullptr) {
        XPLMUnloadObject(proxy.object);
        proxy.object = nullptr;
        char buffer[256] = {};
        std::snprintf(buffer, sizeof(buffer), "XPTO: proxy=%s OBJ unloaded during plugin cleanup.\n", proxy.definition.name);
        Log(buffer);
    }

    proxy.initialized = false;
}

xpto::ProxyState StateForProxy(const RuntimeProxy& proxy) {
    xpto::ProxyState state;
    state.targetName = proxy.definition.name;
    state.objectLoaded = proxy.object != nullptr;
    state.instanceCreated = proxy.instance != nullptr;
    state.visible = proxy.instance != nullptr;
    state.hasPosition = proxy.initialized;
    state.finalLocalPosition.x = proxy.drawInfo.x;
    state.finalLocalPosition.y = proxy.drawInfo.y;
    state.finalLocalPosition.z = proxy.drawInfo.z;
    state.placement = proxy.placement;
    state.rotation = proxy.rotation;
    state.size = proxy.size;
    state.calibrated = proxy.calibration.calibrated;
    state.calibrationOffset = proxy.calibration.offsetPlacement;
    state.calibrationRotationOffset = proxy.calibration.offsetRotation;
    state.correctedPlacement.x = proxy.calibration.calibrated ? proxy.placement.x + proxy.calibration.offsetPlacement.x : proxy.placement.x;
    state.correctedPlacement.y = proxy.calibration.calibrated ? proxy.placement.y + proxy.calibration.offsetPlacement.y : proxy.placement.y;
    state.correctedPlacement.z = proxy.calibration.calibrated ? proxy.placement.z + proxy.calibration.offsetPlacement.z : proxy.placement.z;
    state.correctedRotation.pitch = proxy.calibration.calibrated ? proxy.rotation.pitch + proxy.calibration.offsetRotation.pitch : proxy.rotation.pitch;
    state.correctedRotation.yaw = proxy.calibration.calibrated ? proxy.rotation.yaw + proxy.calibration.offsetRotation.yaw : proxy.rotation.yaw;
    state.correctedRotation.roll = proxy.calibration.calibrated ? proxy.rotation.roll + proxy.calibration.offsetRotation.roll : proxy.rotation.roll;
    state.calibrationPath = g_calibrationPath;
    state.lastExportPath = g_lastExportPath;
    state.lastExportSucceeded = g_lastExportSucceeded;
    return state;
}

std::filesystem::path ExportPath() {
    return XPlaneSystemPath() / "Resources" / "plugins" / "XPTO" / "exports" / "xpto-placement-export.json";
}

void AircraftInfo(char* aircraftFile, int aircraftFileSize, char* aircraftPath, int aircraftPathSize) {
    if (aircraftFileSize > 0) {
        aircraftFile[0] = '\0';
    }
    if (aircraftPathSize > 0) {
        aircraftPath[0] = '\0';
    }

    XPLMGetNthAircraftModel(0, aircraftFile, aircraftPath);
}

void Timestamp(char* output, int outputSize) {
    const std::time_t now = std::time(nullptr);
    std::tm localTime = {};
#ifdef _WIN32
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif
    std::strftime(output, static_cast<size_t>(outputSize), "%Y-%m-%dT%H:%M:%S%z", &localTime);
}

void JsonEscape(std::ofstream& out, const char* value) {
    out << '"';
    for (const char* cursor = value; cursor != nullptr && *cursor != '\0'; ++cursor) {
        if (*cursor == '"' || *cursor == '\\') {
            out << '\\' << *cursor;
        } else if (*cursor == '\n') {
            out << "\\n";
        } else if (*cursor == '\r') {
            out << "\\r";
        } else if (*cursor == '\t') {
            out << "\\t";
        } else {
            out << *cursor;
        }
    }
    out << '"';
}

}  // namespace

namespace xpto {

void CycleSelectedProxyTarget() {
    g_selectedProxyIndex = (g_selectedProxyIndex + 1) % ProxyCount();
    EnsureCalibrationLoaded(SelectedProxy());
    char buffer[256] = {};
    std::snprintf(buffer, sizeof(buffer), "XPTO: selected proxy target changed to %s.\n", SelectedProxy().definition.name);
    Log(buffer);
}

ProxyState GetSelectedProxyState() {
    EnsureCalibrationLoaded(SelectedProxy());
    return StateForProxy(SelectedProxy());
}

void ShowSelectedProxy() {
    EnsureCalibrationLoaded(SelectedProxy());
    ShowProxy(SelectedProxy());
}

void HideSelectedProxy() {
    EnsureCalibrationLoaded(SelectedProxy());
    HideProxy(SelectedProxy());
}

void ResetSelectedProxy() {
    RuntimeProxy& proxy = SelectedProxy();
    EnsureCalibrationLoaded(proxy);
    if (!proxy.initialized) {
        InitializeProxyNearAircraft(proxy);
    }

    const bool wasVisible = proxy.instance != nullptr;

    if (proxy.instance != nullptr) {
        XPLMDestroyInstance(proxy.instance);
        proxy.instance = nullptr;
    }
    if (proxy.object != nullptr) {
        XPLMUnloadObject(proxy.object);
        proxy.object = nullptr;
    }
    proxy.runtimeObjectPath.clear();

    ResetProxyControlState(proxy);
    ApplySavedCalibrationStart(proxy);
    RecomputeDrawInfoFromControlState(proxy);

    if (wasVisible) {
        if (EnsureInstanceCreated(proxy)) {
            ApplyPosition(proxy, "reset");
        }
        return;
    }

    LogState(proxy, "reset hidden");
}

void NudgeSelectedProxy(float dxMeters, float dyMeters, float dzMeters) {
    RuntimeProxy& proxy = SelectedProxy();
    EnsureCalibrationLoaded(proxy);
    if (!EnsureInstanceCreated(proxy)) {
        return;
    }

    proxy.placement.x += dxMeters;
    proxy.placement.y += dyMeters;
    proxy.placement.z += dzMeters;

    ApplyPosition(proxy, "translated");
}

void RotateSelectedProxy(ProxyRotationAxis axis, float deltaDegrees) {
    RuntimeProxy& proxy = SelectedProxy();
    EnsureCalibrationLoaded(proxy);
    if (!EnsureInstanceCreated(proxy)) {
        return;
    }

    if (axis == ProxyRotationAxis::Roll) {
        proxy.rotation.roll += deltaDegrees;
    } else if (axis == ProxyRotationAxis::Pitch) {
        proxy.rotation.pitch += deltaDegrees;
    } else {
        proxy.rotation.yaw += deltaDegrees;
    }

    ApplyPosition(proxy, "rotated");
}

void ResizeSelectedProxy(ProxySizeAxis axis, float deltaMeters) {
    RuntimeProxy& proxy = SelectedProxy();
    EnsureCalibrationLoaded(proxy);
    if (!proxy.initialized) {
        InitializeProxyNearAircraft(proxy);
    }

    const bool wasVisible = proxy.instance != nullptr;

    if (axis == ProxySizeAxis::Width) {
        proxy.size.width += deltaMeters;
        if (proxy.size.width < 0.01f) {
            proxy.size.width = 0.01f;
        }
        proxy.size.scaleX = proxy.size.width / proxy.definition.baseWidth;
    } else {
        proxy.size.height += deltaMeters;
        if (proxy.size.height < 0.01f) {
            proxy.size.height = 0.01f;
        }
        proxy.size.scaleY = proxy.size.height / proxy.definition.baseHeight;
    }

    if (proxy.instance != nullptr) {
        XPLMDestroyInstance(proxy.instance);
        proxy.instance = nullptr;
    }
    if (proxy.object != nullptr) {
        XPLMUnloadObject(proxy.object);
        proxy.object = nullptr;
    }

    if (!WriteRuntimeProxyObj(proxy)) {
        proxy.runtimeObjectPath.clear();
    }

    if (wasVisible) {
        if (EnsureInstanceCreated(proxy)) {
            ApplyPosition(proxy, "resized");
        }
    } else {
        LogState(proxy, "resized hidden");
    }
}

void CalibrateSelectedProxyHere() {
    RuntimeProxy& proxy = SelectedProxy();
    EnsureCalibrationLoaded(proxy);
    if (!proxy.initialized) {
        InitializeProxyNearAircraft(proxy);
    }

    proxy.calibration.exists = true;
    proxy.calibration.calibrated = true;
    proxy.calibration.proxyPlacement = proxy.placement;
    proxy.calibration.proxyRotation = proxy.rotation;
    proxy.calibration.offsetPlacement.x = proxy.calibration.baselinePlacement.x - proxy.placement.x;
    proxy.calibration.offsetPlacement.y = proxy.calibration.baselinePlacement.y - proxy.placement.y;
    proxy.calibration.offsetPlacement.z = proxy.calibration.baselinePlacement.z - proxy.placement.z;
    proxy.calibration.offsetRotation.pitch = proxy.calibration.baselineRotation.pitch - proxy.rotation.pitch;
    proxy.calibration.offsetRotation.yaw = proxy.calibration.baselineRotation.yaw - proxy.rotation.yaw;
    proxy.calibration.offsetRotation.roll = proxy.calibration.baselineRotation.roll - proxy.rotation.roll;
    proxy.calibration.size = proxy.size;

    if (!SaveCalibration(proxy)) {
        Log("XPTO: failed to save calibration.\n");
    }
    LogState(proxy, "calibrated here");
}

void ClearSelectedProxyCalibration() {
    RuntimeProxy& proxy = SelectedProxy();
    EnsureCalibrationLoaded(proxy);
    proxy.calibration.exists = true;
    proxy.calibration.calibrated = false;
    proxy.calibration.proxyPlacement = proxy.definition.defaultPlacement;
    proxy.calibration.proxyRotation = proxy.definition.defaultRotation;
    proxy.calibration.offsetPlacement = {};
    proxy.calibration.offsetRotation = {};
    proxy.calibration.size.width = proxy.definition.baseWidth;
    proxy.calibration.size.height = proxy.definition.baseHeight;
    proxy.calibration.size.scaleX = 1.0f;
    proxy.calibration.size.scaleY = 1.0f;

    if (!SaveCalibration(proxy)) {
        Log("XPTO: failed to save cleared calibration.\n");
    }
    LogState(proxy, "calibration cleared");
}

void ExportSelectedProxyPlacement() {
    RuntimeProxy& proxy = SelectedProxy();
    EnsureCalibrationLoaded(proxy);
    if (!proxy.initialized) {
        InitializeProxyNearAircraft(proxy);
    }
    RecomputeDrawInfoFromControlState(proxy);

    const std::filesystem::path exportPath = ExportPath();
    const std::filesystem::path exportDir = exportPath.parent_path();

    g_lastExportSucceeded = false;
    std::snprintf(g_lastExportPath, sizeof(g_lastExportPath), "%s", exportPath.string().c_str());

    char aircraftFile[512] = {};
    char aircraftPath[1024] = {};
    char timestamp[64] = {};
    AircraftInfo(aircraftFile, sizeof(aircraftFile), aircraftPath, sizeof(aircraftPath));
    Timestamp(timestamp, sizeof(timestamp));

    const xpto::ProxyPosition correctedPlacement = {
        proxy.calibration.calibrated ? proxy.placement.x + proxy.calibration.offsetPlacement.x : proxy.placement.x,
        proxy.calibration.calibrated ? proxy.placement.y + proxy.calibration.offsetPlacement.y : proxy.placement.y,
        proxy.calibration.calibrated ? proxy.placement.z + proxy.calibration.offsetPlacement.z : proxy.placement.z};
    const xpto::ProxyRotation correctedRotation = {
        proxy.calibration.calibrated ? proxy.rotation.roll + proxy.calibration.offsetRotation.roll : proxy.rotation.roll,
        proxy.calibration.calibrated ? proxy.rotation.pitch + proxy.calibration.offsetRotation.pitch : proxy.rotation.pitch,
        proxy.calibration.calibrated ? proxy.rotation.yaw + proxy.calibration.offsetRotation.yaw : proxy.rotation.yaw};

    if (!proxy.calibration.calibrated) {
        Log("XPTO: exporting uncalibrated proxy placement; placement equals raw proxy source.\n");
    }

    try {
        std::filesystem::create_directories(exportDir);
        std::ofstream out(exportPath, std::ios::trunc);
        if (!out) {
            Log("XPTO: failed to open placement export JSON for writing.\n");
            return;
        }

        out << std::fixed << std::setprecision(9);
        out << "{\n";
        out << "  \"schema\": \"xpto-placement-export-v1\",\n";
        out << "  \"timestamp\": "; JsonEscape(out, timestamp); out << ",\n";
        out << "  \"target\": {\n";
        out << "    \"id\": "; JsonEscape(out, proxy.definition.id); out << ",\n";
        out << "    \"name\": "; JsonEscape(out, proxy.definition.name); out << "\n";
        out << "  },\n";
        out << "  \"aircraft\": {\n";
        out << "    \"file\": "; JsonEscape(out, aircraftFile); out << ",\n";
        out << "    \"path\": "; JsonEscape(out, aircraftPath); out << ",\n";
        out << "    \"folder\": "; JsonEscape(out, g_aircraftFolder); out << ",\n";
        out << "    \"profile_id\": "; JsonEscape(out, g_profileId); out << "\n";
        out << "  },\n";
        out << "  \"calibrated\": " << (proxy.calibration.calibrated ? "true" : "false") << ",\n";
        out << "  \"calibration_path\": "; JsonEscape(out, g_calibrationPath); out << ",\n";
        out << "  \"placement_note\": \"placement is corrected ACF candidate coordinates; proxy_source_placement is raw developer jig source; debug_final_local is rendered X-Plane local/world only.\",\n";
        if (!proxy.calibration.calibrated) {
            out << "  \"calibration_warning\": \"No calibration record is active; placement currently equals proxy_source_placement.\",\n";
        }
        out << "  \"proxy_source_placement\": {\n";
        out << "    \"x\": " << proxy.placement.x << ",\n";
        out << "    \"y\": " << proxy.placement.y << ",\n";
        out << "    \"z\": " << proxy.placement.z << ",\n";
        out << "    \"pitch\": " << proxy.rotation.pitch << ",\n";
        out << "    \"yaw\": " << proxy.rotation.yaw << ",\n";
        out << "    \"roll\": " << proxy.rotation.roll << "\n";
        out << "  },\n";
        out << "  \"calibration_offset\": {\n";
        out << "    \"x\": " << proxy.calibration.offsetPlacement.x << ",\n";
        out << "    \"y\": " << proxy.calibration.offsetPlacement.y << ",\n";
        out << "    \"z\": " << proxy.calibration.offsetPlacement.z << ",\n";
        out << "    \"pitch\": " << proxy.calibration.offsetRotation.pitch << ",\n";
        out << "    \"yaw\": " << proxy.calibration.offsetRotation.yaw << ",\n";
        out << "    \"roll\": " << proxy.calibration.offsetRotation.roll << "\n";
        out << "  },\n";
        out << "  \"placement\": {\n";
        out << "    \"x\": " << correctedPlacement.x << ",\n";
        out << "    \"y\": " << correctedPlacement.y << ",\n";
        out << "    \"z\": " << correctedPlacement.z << ",\n";
        out << "    \"pitch\": " << correctedRotation.pitch << ",\n";
        out << "    \"yaw\": " << correctedRotation.yaw << ",\n";
        out << "    \"roll\": " << correctedRotation.roll << "\n";
        out << "  },\n";
        out << "  \"size\": {\n";
        out << "    \"width\": " << proxy.size.width << ",\n";
        out << "    \"height\": " << proxy.size.height << ",\n";
        out << "    \"scale_x\": " << proxy.size.scaleX << ",\n";
        out << "    \"scale_y\": " << proxy.size.scaleY << "\n";
        out << "  },\n";
        out << "  \"debug_final_local\": {\n";
        out << "    \"x\": " << proxy.drawInfo.x << ",\n";
        out << "    \"y\": " << proxy.drawInfo.y << ",\n";
        out << "    \"z\": " << proxy.drawInfo.z << ",\n";
        out << "    \"pitch\": " << proxy.drawInfo.pitch << ",\n";
        out << "    \"heading\": " << proxy.drawInfo.heading << ",\n";
        out << "    \"roll\": " << proxy.drawInfo.roll << "\n";
        out << "  }\n";
        out << "}\n";
        out.close();
    } catch (const std::exception&) {
        Log("XPTO: exception while exporting placement JSON.\n");
        return;
    }

    g_lastExportSucceeded = true;
    char buffer[640] = {};
    std::snprintf(buffer, sizeof(buffer), "XPTO: exported proxy placement to %s\n", g_lastExportPath);
    Log(buffer);
}

void DestroyAllProxies() {
    for (int i = 0; i < ProxyCount(); ++i) {
        DestroyProxy(g_proxies[i]);
    }
}

}  // namespace xpto

