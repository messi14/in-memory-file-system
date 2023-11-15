#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>

class Directory;
class File;

class Node {
protected:
  enum class NodeType { FILE, DIRECTORY };

private:
  std::string m_name;
  Directory *m_parent;
  NodeType m_nodeType;

public:
  Node(const std::string &name, Directory *parent, NodeType nodeType)
      : m_name(name), m_parent(parent), m_nodeType(nodeType) {}

  const std::string &getName() const { return m_name; }
  const NodeType &getNodeType() const { return m_nodeType; }
  Directory *getParent() { return m_parent; }
};

class File : public Node {
public:
  File(const std::string &name, Directory *parent)
      : Node(name, parent, NodeType::FILE) {}
};

class Directory : public Node {

public:
  Directory(const std::string &name, Directory *parent)
      : Node(name, parent, NodeType::DIRECTORY) {}

  void createNode(const std::string &name, bool isDirectory) {
    std::shared_ptr<Node> temp;
    if (isDirectory) {
      temp = make_shared<Directory>(name, this);
    } else {
      temp = make_shared<File>(name, this);
    }

    // Add file/directory inside a lock to avoid race conditions
    {
      std::unique_lock<std::mutex> lck(m_mtx);
      if (m_children.count(name)) {
        throw std::runtime_error("File/Directory already exists.");
      }

      m_children[name] = temp;
    }
  }

  std::string printWorkingDirectory() {
    Directory *temp = getParent();
    std::string pwd = getName();
    while (temp) {
      pwd = temp->getName() + (temp->getParent() == nullptr ? "" : "/") + pwd;
      temp = temp->getParent();
    }

    return pwd;
  }

  Node *findChild(std::string &path) {
    if (path.empty())
      return this;

    std::stringstream ss(path);
    std::string value;
    Node *temp = this;
    while (getline(ss, value, '/')) {
      if (temp && temp->getNodeType() == NodeType::DIRECTORY) {
        Directory *directory = static_cast<Directory *>(temp);
        if (directory->m_children.count(value)) {
          temp = directory->m_children[value].get();
        }
      } else {
        throw std::runtime_error("Not a valid path.");
      }
    }

    return temp;
  }

private:
  std::mutex m_mtx;
  std::unordered_map<std::string, std::shared_ptr<Node>> m_children;
};

class FileSystem {
private:
  Directory *root;
  Directory *current;

public:
  FileSystem() {
    root = new Directory("/", nullptr);
    current = root;
  }

  void createDirectory(std::string name) {
    current->createNode(name, true /* isDirectory */);
  }

  void createFile(std::string name) {
    current->createNode(name, false /* isDirectory */);
  }

  void changeDirectory(std::string path) {
    if (path.empty())
      return;

    if (path.starts_with("/")) {
      std::string temp = path.substr(1);
      current = static_cast<Directory *>(root->findChild(temp));
    } else {
      current = static_cast<Directory *>(current->findChild(path));
    }
  }

  std::string printWorkingDirectory() {
    return current->printWorkingDirectory();
  }
};

int main(int argc, const char *argv[]) {
  // insert code here...
  std::cout << "Hello, World!\n";
  FileSystem *fs = new FileSystem();

  std::cout << fs->printWorkingDirectory() << std::endl;
  fs->createDirectory("hello");
  fs->createDirectory("hello1");
  fs->createDirectory("hello2");

  fs->changeDirectory("hello1");
  std::cout << fs->printWorkingDirectory() << std::endl;

  fs->createDirectory("world1");
  fs->changeDirectory("world1");
  std::cout << fs->printWorkingDirectory() << std::endl;

  fs->changeDirectory("/hello2");
  std::cout << fs->printWorkingDirectory() << std::endl;

  fs->createDirectory("world2");

  fs->changeDirectory("/");
  std::cout << fs->printWorkingDirectory() << std::endl;

  fs->changeDirectory("/hello2/world2");
  std::cout << fs->printWorkingDirectory() << std::endl;

  return 0;
}
