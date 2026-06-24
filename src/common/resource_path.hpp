/**
 * @file resource_path.hpp
 * @brief 运行时资源路径解析
 */

#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace smrcore::peripherals::internal
{

std::filesystem::path GetCurrentExecutablePath();

std::vector<std::filesystem::path> GetResourceRootCandidates();

std::filesystem::path ResolveResourcePath(const std::filesystem::path &resource_path);

std::string ResolveResourcePathString(const std::string &resource_path);

} // namespace smrcore::peripherals::internal
