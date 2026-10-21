# Host-side audit: print real atlas occupancy on every build, and rebuild the
# auditor only when translations, story text, fonts, or the audit itself change.
FONT_ATLAS_AUDIT_BIN := $(PWD)/build/tools/font_atlas_coverage
FONT_ATLAS_AUDIT_FONTS := \
    $(PWD)/assets/files/Roboto-Regular.ttf \
    $(PWD)/assets/files/RobotoMono-Regular.ttf \
    $(PWD)/assets/files/NotoSans-Regular.ttf \
    $(PWD)/assets/files/NotoSansSC-Regular.ttf \
    $(PWD)/assets/files/NotoSansJP-Regular.ttf

$(FONT_ATLAS_AUDIT_BIN): $(PWD)/tests/test_font_atlas_coverage.cpp $(PWD)/storyline.h $(PWD)/tegel/txl_runtime.h $(TXL_GENERATED) $(FONT_ATLAS_AUDIT_FONTS)
	@mkdir -p $(dir $@)
	c++ -std=c++20 -I$(PWD) -I$(PWD)/build/_deps/stb-src $< -o $@

.PHONY: font-audit
font-audit: $(FONT_ATLAS_AUDIT_BIN)
	@$(FONT_ATLAS_AUDIT_BIN)
