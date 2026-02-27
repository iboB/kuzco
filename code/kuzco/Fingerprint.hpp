#pragma once
#include <itlib/ref_ptr.hpp>

namespace kuzco {

// Fingerprints are a weak reference to Nodes which can be used if a Node/Detached is the same as a
// previously observed one, without keeping it alive (and all of its resource usage)
// They, however cannot be locked as std::weak_ptr
// This would introduce potentially concurrent ref count bumps from 1 to 2 which breaks Node::unique()
//
// DANGER!
//
// Currently nodes are implemented through std::shared_ptr in order to maintain compatibility with
// existing/external APIs. Therefore Fingerprint is implemented through std::weak_ptr.
// Since nodes rely on `unique()` to decide when to copy and unique is basically `use_count() == 1`,
// a Fingerprint to a node will not affect unique.
// Thus, a node can change and if only fingerprints are pointing to it, they will not be able to
// detect the change.
// Ideally `unique` would check both the strong and weak ref counts, (strong==1 && weak==0),
// but std::shared_ptr hides the weak ref count.
//
// The only solution to this problem is to use a custom shared pointer, say xmem::shared_ptr
// (https://github.com/iboB/xmem), but this will break the compatibility with existing/external APIs
//
// So, it is perhaps part of the future to switch to xmem and ditch std::shared_ptr, but for now
// USE FINGERPRINTS ONLY WHEN YOU KNOW WHAT YOU ARE DOING
//
// An example of safe use is on immutable nodes, which are always copied on write and don't make use
// of the `unique()` optimization. Such is the case when using `NodeTransaction`. In a transaction
// the node is never unique as a restore state is always kept.
//
// Other safe uses may exist.
//
// For cases when you're not sure that the use is safe (or when you know it's not) for now you must
// use alternative mechanisms for fingerprinting like revisions or hashes, or just keep a strong ref
// (Detached) and sacrifice the resources.

class Fingerprint {
    std::weak_ptr<const void> m_fp;
public:
    Fingerprint() noexcept = default;

    Fingerprint(const Fingerprint&) = default;
    Fingerprint& operator=(const Fingerprint&) = default;
    Fingerprint(Fingerprint&&) noexcept = default;
    Fingerprint& operator=(Fingerprint&&) noexcept = default;

    template <typename U>
    Fingerprint(const itlib::ref_ptr<U>& ptr) noexcept
        : m_fp(ptr._as_shared_ptr_unsafe())
    {}
    template <typename U>
    Fingerprint& operator=(const itlib::ref_ptr<U>& ptr) noexcept {
        m_fp = ptr._as_shared_ptr_unsafe();
        return *this;
    }

    explicit operator bool() const noexcept {
        std::weak_ptr<void> empty;
        return m_fp.owner_before(empty) || empty.owner_before(m_fp);
    }

    void reset() noexcept {
        m_fp.reset();
    }

    bool sameAs(const Fingerprint& other) const noexcept {
        return !m_fp.owner_before(other.m_fp)
            && !other.m_fp.owner_before(m_fp);
    }

    template <typename U>
    bool sameAs(const itlib::ref_ptr<U>& ptr) const noexcept {
        return !m_fp.owner_before(ptr._as_shared_ptr_unsafe())
            && !ptr._as_shared_ptr_unsafe().owner_before(m_fp);
    }

    bool operator==(const Fingerprint& other) const noexcept {
        return sameAs(other);
    }
    bool operator!=(const Fingerprint& other) const noexcept {
        return !sameAs(other);
    }
};

template <typename T>
bool operator==(const itlib::ref_ptr<T>& a, const Fingerprint& b) noexcept {
    return b.sameAs(a);
}
template <typename T>
bool operator!=(const itlib::ref_ptr<T>& a, const Fingerprint& b) noexcept {
    return !b.sameAs(a);
}
template <typename T>
bool operator==(const Fingerprint& a, const itlib::ref_ptr<T>& b) noexcept {
    return a.sameAs(b);
}
template <typename T>
bool operator!=(const Fingerprint& a, const itlib::ref_ptr<T>& b) noexcept {
    return !a.sameAs(b);
}

} // namespace kuzco
