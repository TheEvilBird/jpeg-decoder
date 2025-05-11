#include <huffman.h>
#include <queue>

class HuffmanTree::Impl {
    struct Node {
        uint8_t value = 0;
        bool terminal = false;
        std::shared_ptr<Node> left;
        std::shared_ptr<Node> right;
    };

public:
    Impl() {
        root = std::make_shared<Node>();
        vertex = root;
    }

    void Build(const std::vector<uint8_t> &code_lengths, const std::vector<uint8_t> &values) {
        if (code_lengths.size() > 16) {
            throw std::invalid_argument("Huffman too big");
        }
        if (root != nullptr) {
            Delete(root);
            root.reset();
            root = std::make_shared<Node>();
            vertex = root;
        }
        size_t max_depth = 0;
        for (size_t i = 0; i < code_lengths.size(); ++i) {
            if (code_lengths[i]) {
                max_depth = i + 1;
            }
        }
        size_t ptr_values = 0;
        // size_t ptr_depth = 0;
        // size_t depth = 0;
        std::queue<std::pair<std::shared_ptr<Node>, size_t>> q;
        q.emplace(root, 0);
        std::vector<uint8_t> code_lengths_done(code_lengths.size(), 0);

        while (!q.empty()) {
            auto [v, d] = q.front();
            q.pop();
            if (d > 0 && code_lengths_done[d - 1] < code_lengths[d - 1]) {
                if (ptr_values == values.size()) {
                    throw std::invalid_argument("No value to store");
                }
                v->value = values[ptr_values++];
                v->terminal = true;
                ++code_lengths_done[d - 1];
                continue;
            }
            if (d + 1 > max_depth) {
                continue;
            }
            if (v->left == nullptr) {
                v->left = std::make_shared<Node>();
            }
            q.emplace(v->left, d + 1);
            if (v->right == nullptr) {
                v->right = std::make_shared<Node>();
            }
            q.emplace(v->right, d + 1);
        }
        for (size_t i = 0; i < code_lengths.size(); ++i) {
            if (code_lengths_done[i] < code_lengths[i]) {
                throw std::invalid_argument("Too much vertices");
            }
        }
    }

    bool Move(bool bit, int &value) {
        if (!bit) {
            if (vertex->left == nullptr) {
                throw std::invalid_argument("Can't move");
            }
            vertex = vertex->left;
            if (vertex->terminal) {
                value = vertex->value;
                vertex = root;
                return true;
            }
        } else {
            if (vertex->right == nullptr) {
                throw std::invalid_argument("Can't move");
            }
            vertex = vertex->right;
            if (vertex->terminal) {
                value = vertex->value;
                vertex = root;
                return true;
            }
        }
        return false;
    }

    void Delete(std::shared_ptr<Node> v) {
        if (v == nullptr) {
            return;
        }
        Delete(v->left);
        Delete(v->right);
        v.reset();
    }

    std::shared_ptr<Node> root;
    std::shared_ptr<Node> vertex;
};

HuffmanTree::HuffmanTree() {
    impl_ = std::make_unique<Impl>();
}

void HuffmanTree::Build(const std::vector<uint8_t> &code_lengths,
                        const std::vector<uint8_t> &values) {
    impl_->Build(code_lengths, values);
}

bool HuffmanTree::Move(bool bit, int &value) {
    return impl_->Move(bit, value);
}

HuffmanTree::HuffmanTree(HuffmanTree &&) = default;

HuffmanTree &HuffmanTree::operator=(HuffmanTree &&) = default;

HuffmanTree::~HuffmanTree() = default;
