// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>

yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
  ec = new extent_client(extent_dst);
  lc = new lock_client(lock_dst);
  if (ec->put(1, "") != extent_protocol::OK)
      printf("error init root dir\n"); // XYB: init root dir
}


yfs_client::inum
yfs_client::n2i(std::string n)
{
    std::istringstream ist(n);
    unsigned long long finum;
    ist >> finum;
    return finum;
}

std::string
yfs_client::filename(inum inum)
{
    std::ostringstream ost;
    ost << inum;
    return ost.str();
}

bool
yfs_client::isfile(inum inum)
{
    lc->acquire(inum);
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        lc->release(inum);
        return false;
    }

    if (a.type == extent_protocol::T_FILE) {
        printf("isfile: %lld is a file\n", inum);
        lc->release(inum);
        return true;
    } 
    printf("isfile: %lld is not a file\n", inum);
    lc->release(inum);
    return false;
}
/** Your code here for Lab...
 * You may need to add routines such as
 * readlink, issymlink here to implement symbolic link.
 * 
 * */
bool yfs_client::issymlink(inum inum)
{
    // lc->acquire(inum);
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        // lc->release(inum);
        return false;
    }

    if (a.type == extent_protocol::T_SYMLK) {
        printf("isfile: %lld is a symlink\n", inum);
        // lc->release(inum);
        return true;
    } 
    printf("isfile: %lld is not a symlink\n", inum);
    // lc->release(inum);
    return false;
}

bool
yfs_client::isdir(inum inum)
{
    // lc->acquire(inum);
    // Oops! is this still correct when you implement symlink?
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        // lc->release(inum);
        return false;
    }

    if (a.type == extent_protocol::T_DIR) {
        printf("isfile: %lld is a dir\n", inum);
        // lc->release(inum);
        return true;
    } 
    printf("isfile: %lld is not a dir\n", inum);
    // lc->release(inum);
    return false;
}

// encode
std::string yfs_client::fxxkyou(std::string &fxxk)
{
    std::string buf = "";
    for (uint i = 0; i < fxxk.size(); i++)
    {
        char c =  'a' + fxxk[i] / 16;
        buf += c;
        c = 'a' + fxxk[i] % 16;
        buf += c;
    }
    return buf;
}

// uncode
std::string yfs_client::youfxxk(std::string &fxxk)
{
    std::string buf = "";
    for (uint i = 0; i < fxxk.size(); i += 2)
    {
        char c = (fxxk[i] - 'a') * 16 + fxxk[i + 1] - 'a'; 
        buf += c;
    }
    return buf;
}

int
yfs_client::getfile(inum inum, fileinfo &fin)
{
    int r = OK;

    printf("getfile %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }

    fin.atime = a.atime;
    fin.mtime = a.mtime;
    fin.ctime = a.ctime;
    fin.size = a.size;
    printf("getfile %016llx -> sz %llu\n", inum, fin.size);

release:
    return r;
}

int
yfs_client::getsymlink(inum inum, symlinkinfo &sin1)
{
    int r = OK;
    printf("getlink %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }
    sin1.atime = a.atime;
    sin1.mtime = a.mtime;
    sin1.ctime = a.ctime;
    sin1.size = a.size;
    printf("getlink %016llx -> sz %llu\n", inum, sin1.size);
release:
    return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
    int r = OK;

    printf("getdir %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }
    din.atime = a.atime;
    din.mtime = a.mtime;
    din.ctime = a.ctime;

release:
    return r;
}


#define EXT_RPC(xx) do { \
    if ((xx) != extent_protocol::OK) { \
        printf("EXT_RPC Error: %s:%d \n", __FILE__, __LINE__); \
        r = IOERR; \
        goto release; \
    } \
} while (0)

// Only support set size of attr
int
yfs_client::setattr(inum ino, size_t size)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: get the buf of inode ino, and modify its buf
     * according to the size (<, =, or >) buf length.
     */
    std::string buf;
    extent_protocol::attr a;
    extent_protocol::status ret;
    if ((ret = ec->getattr(ino, a)) != extent_protocol::OK) {
      printf("error getting attr, return not OK\n");
      return ret;
    }
    ec->get(ino, buf);
    if (a.size < size) 
    {
      buf += std::string(size - a.size, '\0');
    } 
    else if (a.size > size) 
    {
      buf = buf.substr(0, size);
    }
    ec->put(ino, buf);
    return r;
}

int
yfs_client::create(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: lookup is what you need to check if file exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */
    lc->acquire(parent);
    bool found = false;
    inum inode;
    std::string buf;
    lookup(parent, name, found, inode);
    if (found == true)
    {
        lc->release(parent);
        return EXIST;
    }

    //mkfile
    ec->create(extent_protocol::T_FILE, ino_out);
    // modify the parent
    ec->get(parent, buf);
    std::string name_buf = "";
    uint off = 0;
    while (name[off] != '\0')
    {
        name_buf += name[off];
        off++;
    }
    buf += fxxkyou(name_buf);
    buf += " ";
    buf += filename(ino_out);
    buf += " ";

    // std::ofstream ofs("Log.me", std::ios::out);
    // ofs << buf;
    // ofs.close();

    // ofs.open("Log1.me", std::ios::out | std::ios::app);
    // ofs << name_buf << ";" << filename(ino_out) << "\tfile" << "\n";
    // ofs.close();

    ec->put(parent, buf);
    lc->release(parent);
    return r;
}

int
yfs_client::mkdir(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: lookup is what you need to check if directory exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */
    lc->acquire(parent);
    bool found = false;
    inum inode;
    std::string buf;
    lookup(parent, name, found, inode);
    if (found == true)
    {
        lc->release(parent);
        return EXIST;
    }

    //mkfile
    ec->create(extent_protocol::T_DIR, ino_out);
    // modify the parent
    ec->get(parent, buf);
    std::string name_buf = "";
    uint off = 0;
    while (name[off] != '\0')
    {
        name_buf += name[off];
        off++;
    }
    buf += fxxkyou(name_buf);
    buf += " ";
    buf += filename(ino_out);
    buf += " ";

    // std::ofstream ofs("Log.me", std::ios::out);
    // ofs << buf;
    // ofs.close();

    // ofs.open("Log1.me", std::ios::out | std::ios::app);
    // ofs << name_buf << ";" << filename(ino_out) << "\tfile" << "\n";
    // ofs.close();

    ec->put(parent, buf);
    lc->release(parent);
    return r;
}

int
yfs_client::lookup(inum parent, const char *name, bool &found, inum &ino_out)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: lookup file from parent dir according to name;
     * you should design the format of directory buf.
     */
    std::string buf;
    extent_protocol::attr attr;

    ec->getattr(parent, attr);
    if (attr.type != extent_protocol::T_DIR)
    {
        // maybe wrong -> typeerr
        return EXIST;
    }

    ec->get(parent, buf);

    std::stringstream ss(buf);
    std::string fileName = "";
    std::string inode = "";
    int leng = buf.size();
    for (uint i = 0; i < leng; i += fileName.size() + inode.size() + 2)
    {
        ss >> fileName >> inode;
        std::string name_buf = youfxxk(fileName);
        char buf_name[name_buf.size() + 1];
        for (int i = 0; i < name_buf.size(); i++)
        {
            buf_name[i] = name_buf[i];
        }
        buf_name[name_buf.size()] = '\0';
        if (strcmp(buf_name, name) == 0)
        {
            ino_out = n2i(inode);
            found = true;
            return r;
        }
    }

    found = false;
    return IOERR;
}

//err
int
yfs_client::readdir(inum dir, std::list<dirent> &list)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: you should parse the dirctory buf using your defined format,
     * and push the dirents to the list.
     */
    lc->acquire(dir);
    std::string buf;
    std::ofstream ofs("log_readDir.me", std::ios::out);
    if (!isdir(dir))
    {
        return EXIST;
    }

    ec->get(dir, buf);
    // parse
    std::stringstream ss(buf);
    std::string fileName = "";
    std::string inode = "";
    uint leng = buf.size();
    // notice
    // ofs << buf;
    for (uint i = 0; i < leng; i += fileName.size() + inode.size() + 2)
    {
        ss >> fileName >> inode;
        std::string name_buf = youfxxk(fileName);

        dirent buf_d;
        buf_d.name = name_buf;
        buf_d.inum = n2i(inode);
        list.push_back(buf_d);
        // ofs << buf_d.name << "\t" << buf_d.inum << "\t" << isfile(buf_d.inum) << "\n";
    }
    lc->release(dir);
    return r;
}

// canshu unknown
int
yfs_client::read(inum ino, size_t size, off_t off, std::string &data)
{
    int r = OK;
    std::fstream ofs("logRead.me", std::ios::out | std::ios::app);

    /*
     * your lab2 code goes here.
     * note: read using ec->get().
     */
    lc->acquire(ino);
    ec->get(ino, data);
    if (off <= data.size())
    {
        if (off + size <= data.size())
        {
            data = data.substr(off, size);
        } 
        else
        {
            uint len = data.size();
            data = data.substr(off, len - off);
        }
    }
    else 
      data = "";
    lc->release(ino);
    return r;
}

// time
int
yfs_client::write(inum ino, size_t size, off_t off, const char *data,
        size_t &bytes_written)
{
    int r = OK;
    std::fstream ofs("logWrite.me", std::ios::out);

    /*
     * your lab2 code goes here.
     * note: write using ec->put().
     * when off > length of original file, fill the holes with '\0'.
     */
    lc->acquire(ino);
    std::string buf;
    std::string dada_buf = std::string(data, size);
    ec->get(ino, buf);
    ofs << "inum:\t" << ino << "\n";
    ofs << "original size:\t" << buf.size() << "\n";
    if (off > buf.size())
    {
        buf.resize(off + size, '\0');
        bytes_written = size;
    }
    // resize
    else if (size + off > buf.size())
    {
        buf.resize(off + size, '\0');
        bytes_written = size;
    }
    else
    {
        bytes_written = size;
    }
    ofs << "new size:\t" << buf.size() << "\n";
    buf.replace(off, size, dada_buf);
    ec->put(ino, buf);
    ofs << "size:\t" << size << "\n"
            << "off:\t" << off << "\n" 
            << "bytes_written:\t" << bytes_written << "\n"
            << buf << "\n"
            << dada_buf << "\n"
            << "\n\n\n";
    lc->release(ino);
    return r;
}

int yfs_client::unlink(inum parent,const char *name)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: you should remove the file using ec->remove,
     * and update the parent directory buf.
     */
    lc->acquire(parent);
    bool flag = false;
    std::string buf;
    std::fstream ofs("logUnlink.me", std::ios::out);
    std::string result = "";
    ec->get(parent, buf);
    std::string fileName = "";
    std::string node = "";
    std::stringstream ss(buf);
    for (uint i = 0; i < buf.size(); i +=fileName.size() + node.size() + 2)
    {
        ss >> fileName >> node;
        std::string realName;
        realName = youfxxk(fileName);
        char name_buf[realName.size() + 1];
        for (uint j = 0; j < realName.size(); j++)
        {
            name_buf[j] = realName[j];
        }
        name_buf[realName.size()] = '\0';
        if (strcmp(name_buf, name) == 0)
        {
            ec->remove(n2i(node));
            flag = true;
        }
        else
        {
            result += fileName;
            result += " ";
            result += node;
            result += " ";
        }
    }
    if (flag == false)
    {
        lc->release(parent);
        return ENOENT;
    }
    ec->put(parent, result);
    lc->release(parent);
    return r;
}

// error, unused
int yfs_client::pathToInum(const char *path, inum dir)
{
    uint off = 0;
    bool flag = false;
    inum inode;
    while(path[off] != '\0')
    {
        if (path[off] == '/')
        {
            flag = true;
            break;
        }
        off++;
    }

    if (flag == false)
    {
        lookup(dir, path, flag, inode);
        if (flag == true)
        {
            return inode;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        off = 0;
        std::string buf = "";
        // get first path
        while(path[off] != '\0')
        {
            if (path[off] != '/')
            {
                buf += path[off];
            }
            else
            {
                break;
            }
            off++;
        }
        return OK;
    }
}

// error, unused
int yfs_client::allPath(const char *name, inum parent)
{
    // general path
    if (name[0] == '/')
    {

    }
    else
    {

    }
    return OK;
}

// create the soft link
int yfs_client::symlink(const char *link, inum parent, const char *name, inum &ino)
{
    int r = OK;
    lc->acquire(parent);
    std::fstream ofs("logsymlk.me", std::ios::out);
    bool found = false;
    inum inode;
    std::string buf;
    lookup(parent, name, found, inode);
    if (found == true)
    {
        lc->release(parent);
        return EXIST;
    }

    //ln
    ec->create(extent_protocol::T_SYMLK, ino);
    // modify the parent
    ec->get(parent, buf);
    std::string name_buf = "";
    uint off = 0;
    while (name[off] != '\0')
    {
        name_buf += name[off];
        off++;
    }
    buf += fxxkyou(name_buf);
    buf += " ";
    buf += filename(ino);
    buf += " ";
    ec->put(parent, buf);
    ofs << buf << "\n";

    // change the name's node's content
    buf = "";
    off = 0;
    while (link[off] != '\0')
    {
        buf += link[off];
        off++;
    }
    ofs << buf << "\n";
    ec->put(ino, buf);
    lc->release(parent);
    return r;
}

// ino: inum of the b;
// b->ln -s a b
// get the content of the inode
int yfs_client::readlink(inum ino, std::string &result)
{
    std::fstream ofs("logReadLk.me", std::ios::out);
    int r = OK;
    ec->get(ino, result);
    ofs << result << "\n";
    return r;

}

