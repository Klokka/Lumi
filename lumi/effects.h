//
// lumi/effects.h
// The presently meager effects API.
//
void lumi_gaussian_blur_filter(cairo_t *, VALUE, lumi_place *);
void lumi_shadow_filter(cairo_t *, VALUE, lumi_place *);
void lumi_glow_filter(cairo_t *, VALUE, lumi_place *);
