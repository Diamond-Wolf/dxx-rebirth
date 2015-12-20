/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license.
 * See COPYING.txt for license details.
 */

#pragma once

#include <physfs.h>
#include "maths.h"

#include <cstdint>
#include "fwd-valptridx.h"
#include "segnum.h"

#include "dxxsconf.h"
#include "compiler-array.h"

namespace dcx {
constexpr std::size_t MAX_SEGMENTS = 9000;
}
#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
namespace dsx {
struct segment;
DXX_VALPTRIDX_DECLARE_GLOBAL_SUBTYPE(segment, seg, Segments, MAX_SEGMENTS);

static constexpr valptridx<segment>::magic_constant<0xfffe> segment_exit{};
static constexpr valptridx<segment>::magic_constant<0xffff> segment_none{};
static constexpr valptridx<segment>::magic_constant<0> segment_first{};
}
#endif

namespace dcx {
const std::size_t MAX_VERTICES_PER_SEGMENT = 8;
const std::size_t MAX_SIDES_PER_SEGMENT = 6;
const std::size_t MAX_VERTICES_PER_POLY = 4;

const std::size_t MAX_SEGMENTS_ORIGINAL = 900;
const std::size_t MAX_SEGMENT_VERTICES_ORIGINAL = 4*MAX_SEGMENTS_ORIGINAL;
const std::size_t MAX_SEGMENT_VERTICES = 4*MAX_SEGMENTS;

typedef uint_fast32_t sidenum_fast_t;

enum sidenum_t : uint8_t
{
	WLEFT = 0,
	WTOP = 1,
	WRIGHT = 2,
	WBOTTOM = 3,
	WBACK = 4,
	WFRONT = 5
};

//normal everyday vertices

const fix DEFAULT_LIGHTING = 0;   // (F1_0/2)

#ifdef EDITOR   //verts for the new segment
const std::size_t NUM_NEW_SEG_VERTICES = 8;
const std::size_t NEW_SEGMENT_VERTICES = MAX_SEGMENT_VERTICES;
const std::size_t MAX_VERTICES = MAX_SEGMENT_VERTICES + NUM_NEW_SEG_VERTICES;
#else           //No editor
const std::size_t MAX_VERTICES = MAX_SEGMENT_VERTICES;
#endif

struct uvl;
enum side_type : uint8_t;

template <int16_t I>
struct wall_magic_constant_t;

struct wallnum_t;
struct side;

}

typedef unsigned segment_type_t;
const segment_type_t SEGMENT_IS_NOTHING = 0;
const segment_type_t SEGMENT_IS_FUELCEN = 1;
const segment_type_t SEGMENT_IS_REPAIRCEN = 2;
const segment_type_t SEGMENT_IS_CONTROLCEN = 3;
const segment_type_t SEGMENT_IS_ROBOTMAKER = 4;
#if defined(DXX_BUILD_DESCENT_I)
const std::size_t MAX_CENTER_TYPES = 5;
#elif defined(DXX_BUILD_DESCENT_II)
typedef unsigned s2f_ambient_t;
const s2f_ambient_t S2F_AMBIENT_WATER = 1;
const s2f_ambient_t S2F_AMBIENT_LAVA = 2;
const segment_type_t SEGMENT_IS_GOAL_BLUE = 5;
const segment_type_t SEGMENT_IS_GOAL_RED = 6;
const std::size_t MAX_CENTER_TYPES = 7;
#endif

namespace dcx {
struct count_segment_array_t;
struct group;

struct vertex;
extern array<vertex, MAX_VERTICES> Vertices;
extern unsigned Num_segments;
extern unsigned Num_vertices;

const std::size_t MAX_EDGES = MAX_VERTICES * 4;

extern const array<array<uint8_t, 4>, MAX_SIDES_PER_SEGMENT> Side_to_verts;       // Side_to_verts[my_side] is list of vertices forming side my_side.
extern const array<array<unsigned, 4>, MAX_SIDES_PER_SEGMENT>  Side_to_verts_int;    // Side_to_verts[my_side] is list of vertices forming side my_side.
extern const array<uint8_t, MAX_SIDES_PER_SEGMENT> Side_opposite;                                // Side_opposite[my_side] returns side opposite cube from my_side.

void segment_side_wall_tmap_write(PHYSFS_file *fp, const side &side);
}
void delete_segment_from_group(segnum_t segment_num, int group_num);
void add_segment_to_group(segnum_t segment_num, int group_num);

#if defined(DXX_BUILD_DESCENT_II)
namespace dsx {
struct delta_light;
struct dl_index;

const std::size_t MAX_DL_INDICES = 500;
const std::size_t MAX_DELTA_LIGHTS = 10000;

const fix DL_SCALE = 2048;    // Divide light to allow 3 bits integer, 5 bits fraction.

extern array<dl_index, MAX_DL_INDICES> Dl_indices;
extern array<delta_light, MAX_DELTA_LIGHTS> Delta_lights;
extern unsigned Num_static_lights;

int subtract_light(vsegptridx_t segnum, sidenum_fast_t sidenum);
int add_light(vsegptridx_t segnum, sidenum_fast_t sidenum);
void clear_light_subtracted();

void segment2_read(vsegptr_t s2, PHYSFS_file *fp);
void segment2_write(vcsegptr_t s2, PHYSFS_file *fp);

void delta_light_read(delta_light *dl, PHYSFS_file *fp);
void delta_light_write(delta_light *dl, PHYSFS_file *fp);

void dl_index_read(dl_index *di, PHYSFS_file *fp);
void dl_index_write(dl_index *di, PHYSFS_file *fp);
}
#endif

namespace dcx {

template <typename T, unsigned bits>
class visited_segment_mask_t;
class visited_segment_bitarray_t;

template <unsigned bits>
class visited_segment_multibit_array_t;

const int side_none = -1;
const int edge_none = -1;

}
