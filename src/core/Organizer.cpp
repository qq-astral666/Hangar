#include "Organizer.h"

#include <map>
#include <set>

namespace hangar {

namespace {

fs::path realPath(const fs::path& p)
{
    std::error_code ec;
    fs::path r = fs::weakly_canonical(p, ec);
    return ec ? p : r;
}

} // namespace

std::vector<Link> plan(const std::vector<Project>& projects, const fs::path& targetRoot)
{
    std::vector<Link> links;
    std::map<fs::path, int> used;   // link path -> count
    for (const Project& p : projects) {
        const fs::path folder = targetRoot / utf8Path(folderName(p.category));
        std::string name = p.name;
        fs::path link = folder / utf8Path(name);
        for (int n = 2; used.count(link); ++n)
            link = folder / utf8Path(name + " (" + std::to_string(n) + ")");
        used[link] = 1;
        links.push_back({ link, p.path });
    }
    return links;
}

OrganizeResult organize(const std::vector<Project>& projects, const fs::path& targetRoot)
{
    OrganizeResult result;
    const std::vector<Link> links = plan(projects, targetRoot);

    std::set<fs::path> wanted;
    for (const Link& l : links)
        wanted.insert(l.link);

    // 1. Clean up our old links: symlinks in category folders that aren't in
    //    the plan any more (project re-categorised, renamed or deleted).
    for (int c = 0; c < kCategoryCount; ++c) {
        const fs::path folder = targetRoot / utf8Path(folderName(Category(c)));
        std::error_code ec;
        if (!fs::is_directory(folder, ec))
            continue;
        std::vector<fs::path> stale;
        for (fs::directory_iterator it(folder, ec), end; it != end && !ec; it.increment(ec)) {
            std::error_code ec2;
            if (fs::is_symlink(it->symlink_status(ec2)) && !wanted.count(it->path()))
                stale.push_back(it->path());
        }
        for (const fs::path& s : stale) {
            std::error_code ec2;
            if (fs::remove(s, ec2))
                ++result.removed;
            else
                result.errors.push_back(s.string() + ": " + ec2.message());
        }
    }

    // 2. Create links.
    for (const Link& l : links) {
        std::error_code ec;
        fs::create_directories(l.link.parent_path(), ec);
        if (ec) {
            result.errors.push_back(l.link.parent_path().string() + ": " + ec.message());
            continue;
        }
        const fs::file_status st = fs::symlink_status(l.link, ec);
        if (fs::exists(st)) {
            if (fs::is_symlink(st) && realPath(fs::read_symlink(l.link, ec)) == realPath(l.target)) {
                ++result.kept;
                continue;
            }
            if (fs::is_symlink(st)) {
                fs::remove(l.link, ec);   // our link, pointing elsewhere
            } else {
                result.errors.push_back(l.link.string() + ": уже есть настоящая папка с таким именем, пропущено");
                continue;
            }
        }
        fs::create_directory_symlink(l.target, l.link, ec);
        if (ec)
            result.errors.push_back(l.link.string() + ": " + ec.message());
        else
            ++result.created;
    }
    return result;
}

int removeLinksTo(const fs::path& targetRoot, const fs::path& project)
{
    int removed = 0;
    const fs::path real = realPath(project);
    for (int c = 0; c < kCategoryCount; ++c) {
        const fs::path folder = targetRoot / utf8Path(folderName(Category(c)));
        std::error_code ec;
        if (!fs::is_directory(folder, ec))
            continue;
        std::vector<fs::path> hits;
        for (fs::directory_iterator it(folder, ec), end; it != end && !ec; it.increment(ec)) {
            std::error_code ec2;
            if (!fs::is_symlink(it->symlink_status(ec2)))
                continue;
            fs::path target = fs::read_symlink(it->path(), ec2);
            if (!ec2 && (target == project || realPath(target) == real))
                hits.push_back(it->path());
        }
        for (const fs::path& h : hits) {
            std::error_code ec2;
            removed += fs::remove(h, ec2) ? 1 : 0;
        }
    }
    return removed;
}

} // namespace hangar
