/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 * 2021 cli345 on GimpChat (Discovering the GEGL GRAPH) (2022, Beaver making the Graph into a GEGL filter
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES


#define TUTORIAL \
" noise-reduction domain-transform n-iterations=5  :\n"\

property_string (string1, _("GEGL 1"), TUTORIAL)
    ui_meta     ("role", "output-extent")



#define TUTORIAL2 \
" gimp:desaturate mode=value  :\n"\

property_string (string2, _("GEGL 2"), TUTORIAL2)
    ui_meta     ("role", "output-extent")

#define TUTORIAL3 \
" invert-gamma rgb-clip  :\n"\

property_string (string3, _("GEGL 3"), TUTORIAL3)
    ui_meta     ("role", "output-extent")

#define TUTORIAL4 \
" domain-transform domain-transform  domain-transform   :\n"\

property_string (string4, _("GEGL 4"), TUTORIAL4)
    ui_meta     ("role", "output-extent")



enum_start (gegl_blend_mode_type2)
  enum_value (GEGL_BLEND_MODE_TYPE_HARDLIGHT, "Hardlight",
              N_("HardLight"))
  enum_value (GEGL_BLEND_MODE_TYPE_MULTIPLY,      "Multiply",
              N_("Multiply"))
  enum_value (GEGL_BLEND_MODE_TYPE_OVERLAY,      "Overlay", 
              N_("Overlay"))
property_enum (blendmode, _("Blend Mode of Lighting and Chroma"),
    GeglBlendModeType2, gegl_blend_mode_type2,
    GEGL_BLEND_MODE_TYPE_HARDLIGHT)
enum_end (GeglBlendModeType2)

property_double (sat, _("Chroma"), 1.3)
    description(_("Scale, strength of effect"))
    value_range (0.0, 15.0)
    ui_range (0.0, 15.0)


property_double (lightness, _("Lightness"), 0.0)
   description  (_("Lightness adjustment"))
   value_range  (-0, 18.0)



property_double (radius1, _("Difference of Gaussian 1"), 1.2)
  value_range (0.0, 4.0)
  ui_range (0.0, 2.0)
  ui_gamma (1.5)
   value_range  (0.500, 2.00)

property_double (radius2, _("Difference of Gaussian 2"), 0.53)
  value_range (0.0, 4.0)
  ui_range (0.0, 2.0)
  ui_gamma (1.5)
   value_range  (0.0, 0.6)



property_int (smooth, _("Domain Smooth Settings"), 3)
  description(_("Number of filtering iterations. "
                "A value between 2 and 4 is usually enough."))
  value_range (1, 5)



property_double (in_low, _("Low Levels input"), 0.007)
    description ( _("Input luminance level to become lowest output"))
    ui_range    (0.002, 0.010)
   value_range  (0.002, 0.010)

property_double (in_high, _("High Levels input"), 0.009)
    description (_("Input luminance level to become white"))
    ui_range    (0.006, 0.030)
    value_range    (0.006, 0.030)

property_int  (mcb, _("Smooth Final Image"), 2)
  description (_("Controls the number of iterations"))
  value_range (0, 4)
  ui_range    (0, 4)



#else

#define GEGL_OP_META
#define GEGL_OP_NAME     photo2cartoon
#define GEGL_OP_C_SOURCE photo2cartoon.c

#include "gegl-op.h"




typedef struct
{
  GeglNode *input;
  GeglNode *nop; 
  GeglNode *nr;
  GeglNode *gegl1;
  GeglNode *dog;
  GeglNode *gegl2;
  GeglNode *levels;
  GeglNode *gegl3;
  GeglNode *hardlight;
  GeglNode *multiply;
  GeglNode *overlay; 
  GeglNode *lightchroma;
  GeglNode *gegl4;
  GeglNode *smooth;
  GeglNode *mcb;  
  GeglNode *output;
}State;

static void
update_graph (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  State *state = o->user_data;
  if (!state) return;

  GeglNode *usethis = state->hardlight; /* the default */
  switch (o->blendmode) {
    case GEGL_BLEND_MODE_TYPE_MULTIPLY: usethis = state->multiply; break;
    case GEGL_BLEND_MODE_TYPE_OVERLAY: usethis = state->overlay; break;
  }
  gegl_node_link_many (state->input, state->nop, state->nr, state->gegl1, state->dog, state->gegl2, state->levels, state->gegl3, usethis, state->gegl4, state->smooth, state->mcb, state->output,  NULL);
  gegl_node_connect_from (usethis, "aux", state->lightchroma, "output");

}

static void attach (GeglOperation *operation)
{
  GeglNode *gegl = operation->node;
GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglNode *input, *output, *nop, *nr, *gegl1, *dog, *gegl2, *levels, *gegl3, *hardlight, *multiply, *overlay, *lightchroma, *gegl4, *smooth, *mcb;

  input    = gegl_node_get_input_proxy (gegl, "input");
  output   = gegl_node_get_output_proxy (gegl, "output");





  nop    = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

  multiply    = gegl_node_new_child (gegl,
                                  "operation", "gegl:multiply",
                                  NULL);


  hardlight    = gegl_node_new_child (gegl,
                                  "operation", "gegl:hard-light",
                                  NULL);

  overlay = gegl_node_new_child (gegl, "operation", "gegl:overlay", "srgb", TRUE, NULL);




  lightchroma    = gegl_node_new_child (gegl,
                                  "operation", "gegl:hue-chroma",
                                  NULL);

  mcb    = gegl_node_new_child (gegl,
                                  "operation", "gegl:mean-curvature-blur",
                                  NULL);

  gegl1    = gegl_node_new_child (gegl,
                                  "operation", "gegl:gegl",
                                  NULL);

  gegl2    = gegl_node_new_child (gegl,
                                  "operation", "gegl:gegl",
                                  NULL);

  gegl3    = gegl_node_new_child (gegl,
                                  "operation", "gegl:gegl",
                                  NULL);


  gegl4    = gegl_node_new_child (gegl,
                                  "operation", "gegl:gegl",
                                  NULL);



  levels    = gegl_node_new_child (gegl,
                                  "operation", "gegl:levels",
                                  NULL);


  nr    = gegl_node_new_child (gegl,
                                  "operation", "gegl:noise-reduction",
                                  NULL);

  dog    = gegl_node_new_child (gegl,
                                  "operation", "gegl:difference-of-gaussians",
                                  NULL);

  smooth    = gegl_node_new_child (gegl,
                                  "operation", "gegl:domain-transform",
                                  NULL);










   gegl_operation_meta_redirect (operation, "in_high", levels, "in-high");
  gegl_operation_meta_redirect (operation, "in_low", levels, "in-low");
  gegl_operation_meta_redirect (operation, "smooth", smooth, "n-iterations");
  gegl_operation_meta_redirect (operation, "sat", lightchroma, "chroma");
  gegl_operation_meta_redirect (operation, "radius1", dog, "radius1");
  gegl_operation_meta_redirect (operation, "radius2", dog, "radius2");
  gegl_operation_meta_redirect (operation, "lightness", lightchroma, "lightness");
  gegl_operation_meta_redirect (operation, "mcb", mcb, "iterations");
  gegl_operation_meta_redirect (operation, "string1", gegl1, "string");
  gegl_operation_meta_redirect (operation, "string2", gegl2, "string");
  gegl_operation_meta_redirect (operation, "string3", gegl3, "string");
  gegl_operation_meta_redirect (operation, "string4", gegl4, "string");






  gegl_node_link_many (input, nop, nr, gegl1, dog, gegl2, levels, gegl3, hardlight, gegl4, smooth, mcb, output, NULL);
  gegl_node_connect_from (hardlight, "aux", lightchroma, "output");
  gegl_node_link_many (nop, lightchroma, NULL);




  /* now save references to the gegl nodes so we can use them
   * later, when update_graph() is called
   */
  State *state = g_malloc0 (sizeof (State));
  state->input = input;
  state->nop = nop;
  state->nr = nr;
  state->gegl1 = gegl1;
  state->dog = dog;
  state->gegl2 = gegl2;
  state->levels = levels;
  state->gegl3 = gegl3;
  state->hardlight = hardlight;
  state->multiply = multiply;
  state->overlay = overlay;
  state->lightchroma = lightchroma;
  state->gegl4 = gegl4;
  state->smooth = smooth;
  state->mcb = mcb;
  state->output = output;

  o->user_data = state;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;
GeglOperationMetaClass *operation_meta_class = GEGL_OPERATION_META_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach = attach;
  operation_meta_class->update = update_graph;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:photo2cartoon",
    "title",       _("Photo to Cartoon"),
    "categories",  "Artistic",
    "reference-hash", "h3af1vv0nyesyeefjf25sb2ac",
    "description", _("GEGL makes a image into a cartoon."
                     ""),
    NULL);
}

#endif
