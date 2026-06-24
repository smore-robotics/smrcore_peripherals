/**
 * @file resource_path.cpp
 * @brief 运行时资源路径解析实现
 */

#include "common/resource_path.hpp"

#include <cstdlib>
#include <system_error>

#ifndef _WIN32
#include <limits.h>
#include <unistd.h>
#endif

namespace smrcore::peripherals::internal
{
namespace
{

constexpr const char *kInstallRoot = "/opt/robot-arm";

bool PathExists(const std::filesystem::path &path)
{
    std::error_code ec;
    return std::filesystem::exists(path, ec);
}

void AddUnique(std::vector<std::filesystem::path> &paths,
               const std::filesystem::path &path)
{
    if (path.empty())
    {
        return;
    }

    std::error_code ec;
    const auto normalized = std::filesystem::weakly_canonical(path, ec);
    const auto candidate = ec ? path.lexically_normal() : normalized;
    for (const auto &existing : paths)
    {
        if (existing == candidate)
        {
            return;
        }
    }
    paths.push_back(candidate);
}

} // namespace

std::filesystem::path GetCurrentExecutablePath()
{
#ifndef _WIN32
    std::string buffer(PATH_MAX, '\0');
    while (true)
    {
        const ssize_t size =
            readlink("/proc/self/exe", buffer.data(), buffer.size());
        if (size < 0)
        {
            return {};
        }
        if (static_cast<std::size_t>(size) < buffer.size())
        {
            buffer.resize(static_cast<std::size_t>(size));
            return std::filesystem::path(buffer);
        }
        buffer.resize(buffer.size() * 2);
    }
#else
    return {};
#endif
}

std::vector<std::filesystem::path> GetResourceRootCandidates()
{
    std::vector<std::filesystem::path> roots;

    if (const char *config_dir = std::getenv("SMRCORE_PERIPHERALS_CONFIG_DIR");
        config_dir != nullptr && config_dir[0] != '\0')
    {
        AddUnique(roots, std::filesystem::path(config_dir));
    }

    const auto exe_path = GetCurrentExecutablePath();
    if (!exe_path.empty() && !exe_path.parent_path().empty())
    {
        AddUnique(roots, exe_path.parent_path());
        AddUnique(roots, exe_path.parent_path() / "..");
        AddUnique(roots, exe_path.parent_path() / "../..");
    }

    AddUnique(roots, kInstallRoot);

    std::error_code ec;
    const auto cwd = std::filesystem::current_path(ec);
    if (!ec)
    {
        AddUnique(roots, cwd);
    }

    return roots;
}

std::filesystem::path
ResolveResourcePath(const std::filesystem::path &resource_path)
{
    if (resource_path.empty())
    {
        return {};
    }

    if (resource_path.is_absolute())
    {
        return PathExists(resource_path) ? resource_path : std::filesystem::path();
    }

    for (const auto &root : GetResourceRootCandidates())
    {
        const auto candidate = root / resource_path;
        if (PathExists(candidate))
        {
            return candidate;
        }
    }

    return {};
}

std::string ResolveResourcePathString(const std::string &resource_path)
{
    return ResolveResourcePath(std::filesystem::path(resource_path)).string();
}

} // namespace smrcore::peripherals::internal
