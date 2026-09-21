# common.mk: only ldac/ is added to core-y unconditionally for A2DP_LDAC_ON=1
# (see src/config/common.mk's "BT features" section) — the rest of
# thirdparty/ is gated by USE_THIRDPARTY (0 for open_source) and not needed.
add_subdirectory(ldac)
