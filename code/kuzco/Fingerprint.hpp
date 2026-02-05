#pragma once
#include <itlib/ref_ptr.hpp>

namespace kuzco {

// Fingerprints are a weak reference to Nodes which can be used if a Node/Detached is the same as a
// previously oberved one, without keeping it alive (and all of its resource usage)
// They, however cannot be locked as std::weak_ptr
// This would introduce potentially concurrent ref count bumps from 1 to 2 which breaks Node::unique()
//
// DANGER!
//
// Currently nodes are implemented through std::shared_ptr in order to maitain compatibility with
// existing/external APIs
// Thus Fingerprint is implemented through std::weak_ptr
// Since nodes rely on `unique()` to decide when to copy and unique is basically `use_count() == 1`,
// So, a Fingerprint to a node will not affect unique.
// Thus, a node can change since there are only fingeprints pointing to it and these fingerprints will
// not be able to detect the change.
// Ideally unique would check both the strong and weak ref counts, (strong==1 && weak==0),
// but std::shared_ptr hides the weak one.
//
// The only solution to this problem is to use a custom shared pointer like, say xmem::shared_ptr
// (https://github.com/iboB/xmem), but this will break the compatibility with existing/external APIs
//
// So, it is prehaps part of the future to switch to xmem and ditch std::shared_ptr, but for now
// USE FINGERPRINTS ONLY WHEN YOU KNOW WHAT YOU ARE DOING
//
// An example of safe use is on immutable nodes, which are always copied on write and don't make use
// of the unique() optiization. Such is the case when using NodeTransaction. In a transaction,
// the node is never unique as a restore state is always kept.
//
// Other safe uses may exist.
//
// For cases when you know the use is not safe, for now one must use alternative mechanisms for fingerprinting
// like revisions or hashes.

template <typename T = void>
class Fingerprint {
    template <typename> friend class Fingerprint;

    std::weak_ptr<const T> m_fp;
public:
    Fingerprint() noexcept = default;

    Fingerprint(const Fingerprint&) = default;
    Fingerprint& operator=(const Fingerprint&) = default;
    Fingerprint(Fingerprint&&) noexcept = default;
    Fingerprint& operator=(Fingerprint&&) noexcept = default;

    template <typename U>
    Fingerprint(Fingerprint<U> fp) noexcept
        : m_fp(std::move(fp.m_fp))
    {}
    template <typename U>
    Fingerprint& operator=(Fingerprint<U> fp) noexcept {
        m_fp = std::move(fp.m_fp);
        return *this;
    }
    template <typename U>
    Fingerprint(const itlib::ref_ptr<U>& ptr) noexcept
        : m_fp(ptr._as_shared_ptr_unsafe())
    {}
    template <typename U>
    Fingerprint& operator=(const itlib::ref_ptr<U>& ptr) noexcept {
        m_fp = ptr._as_shared_ptr_unsafe();
        return *this;
    }

    void reset() noexcept {
        m_fp.reset();
    }

    template <typename U>
    bool sameAs(const Fingerprint<U>& other) const noexcept {
        return !m_fp.owner_before(other.m_fp)
            && !other.m_fp.owner_before(m_fp);
    }

    template <typename U>
    bool sameAs(const itlib::ref_ptr<U>& ptr) const noexcept {
        return !m_fp.owner_before(ptr._as_shared_ptr_unsafe())
            && !ptr._as_shared_ptr_unsafe().owner_before(m_fp);
    }

    template <typename U>
    bool operator==(const Fingerprint<U>& other) const noexcept {
        return sameAs(other);
    }
    template <typename U>
    bool operator!=(const Fingerprint<U>& other) const noexcept {
        return !sameAs(other);
    }
};

template <typename A, typename B>
bool operator==(const itlib::ref_ptr<A>& a, const Fingerprint<B>& b) noexcept {
    return b.sameAs(a);
}
template <typename A, typename B>
bool operator!=(const itlib::ref_ptr<A>& a, const Fingerprint<B>& b) noexcept {
    return !b.sameAs(a);
}
template <typename A, typename B>
bool operator==(const Fingerprint<A>& a, const itlib::ref_ptr<B>& b) noexcept {
    return a.sameAs(b);
}
template <typename A, typename B>
bool operator!=(const Fingerprint<A>& a, const itlib::ref_ptr<B>& b) noexcept {
    return !a.sameAs(b);
}

} // namespace kuzco
