// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#include "LoadRequest.h"

#include "common/dout.h"
#include "common/errno.h"
#include "librbd/Utils.h"
#include "librbd/ImageCtx.h"
#include "librbd/crypto/CryptoImageDispatch.h"
#include "librbd/crypto/CryptoObjectDispatch.h"
#include "librbd/io/ImageDispatcherInterface.h"
#include "librbd/io/ObjectDispatcherInterface.h"

#define dout_subsys ceph_subsys_rbd
#undef dout_prefix
#define dout_prefix *_dout << "librbd::crypto::LoadRequest: " << this \
                           << " " << __func__ << ": "

namespace librbd {
namespace crypto {

using librbd::util::create_context_callback;

template <typename I>
LoadRequest<I>::LoadRequest(
        I* image_ctx, std::unique_ptr<EncryptionFormat<I>> format,
        Context* on_finish) : m_image_ctx(image_ctx),
                              m_format(std::move(format)),
                              m_on_finish(on_finish) {
}

template <typename I>
void LoadRequest<I>::send() {
  if (m_image_ctx->io_object_dispatcher->exists(
          io::OBJECT_DISPATCH_LAYER_CRYPTO)) {
    lderr(m_image_ctx->cct) << "encryption already loaded" << dendl;
    finish(-EEXIST);
    return;
  }

  auto ictx = m_image_ctx;
  while (ictx != nullptr) {
    if (ictx->test_features(RBD_FEATURE_JOURNALING)) {
      lderr(m_image_ctx->cct) << "cannot use encryption with journal."
                              << " image name: " << ictx->name << dendl;
      finish(-ENOTSUP);
      return;
    }
    ictx = ictx->parent;
  }

  auto ctx = create_context_callback<
          LoadRequest<I>, &LoadRequest<I>::finish>(this);
  m_format->load(m_image_ctx, &m_crypto, ctx);
}

template <typename I>
void LoadRequest<I>::finish(int r) {
  if (r == 0) {
    // load crypto layers to image and its ancestors
    auto image_dispatch = CryptoImageDispatch::create(
            m_crypto->get_data_offset());
    auto ictx = m_image_ctx;
    while (ictx != nullptr) {
      auto object_dispatch = CryptoObjectDispatch<I>::create(
              ictx, m_crypto);
      ictx->io_object_dispatcher->register_dispatch(object_dispatch);
      ictx->io_image_dispatcher->register_dispatch(image_dispatch);

      ictx = ictx->parent;
    }
  }

  m_on_finish->complete(r);
  delete this;
}

} // namespace crypto
} // namespace librbd

template class librbd::crypto::LoadRequest<librbd::ImageCtx>;
