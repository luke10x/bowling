# Frame profile graph

The graph at the bottom of the game shows the last 100 **completed** frames,
oldest on the left. Each column is a frame; each colored segment is CPU wall
time spent between two checkpoints in `vtx::loop`. The bars use a fixed 100 ms
scale and clip above it. The faint horizontal lines mark 16.67 ms (60 FPS),
33.33 ms (30 FPS), and 66.67 ms (15 FPS). Press **F3** to show or hide the
graph. It starts on and sits above the 28-pixel footer, including the FPS
label. The clearance scales with display pixel ratio.

| Color | Zone | End checkpoint in `game.cpp` |
| --- | --- | --- |
| `#9A7DE3` | EVT: events/setup | After SDL event handling and modal setup |
| `#35B7C6` | STATE: state/story | Before aim and swing updates |
| `#E97F36` | CTRL: controls | Before the ball model and throw processing |
| `#D9528A` | SCORE: throw/scoring | Before physics interval selection |
| `#DC4242` | PHYS: physics | After `physics_step` |
| `#65B95B` | UPD: post-physics updates | Before offscreen previews |
| `#3F83D8` | FBO: UI previews and decal labels | Before the main world pass |
| `#7D8794` | CLR: scene clear and GL setup | After clearing the world framebuffer |
| `#47C8E7` | AUR: aurora sky | After aurora update and draw |
| `#C485DD` | BG: active level backdrop and water | Before pins |
| `#E3BE39` | PINS: pin meshes and effects | Before bot avatar |
| `#5766D8` | BOT: bot avatar | Before opaque lane pass |
| `#38B87B` | LANE: lane and props | Before ball and chest |
| `#E3655B` | BALL: ball and chest | Before transparent world work |
| `#32A7A1` | TRAN: transparent world | After transparent 3D objects and bot wings |
| `#C56A3C` | CLOG: collectable logic | After coin/gem updates and pickup handling |
| `#96B938` | COIN: world collectables | After coin, gem, and glass block draws |
| `#68A9E0` | SNOW: snow particles | After snow draw |
| `#D960B7` | TRAIL: ball trails and rings | After ball trail and spin ring draws |
| `#DB8F31` | AMBI: ambient particles | After sparks, lane dust, and confetti draws |
| `#784CB6` | FLY: flying rewards | After flying collectables and card drop animation |
| `#D64E62` | DECAL: decals | After the decal batch |
| `#49B5B0` | HUD: controls and screen effects | Before Clay UI work |
| `#A9764B` | PREP: UI preparation | Before `Clay_BeginLayout` |
| `#6BA8CF` | LAY: UI layout | Before `Clay_EndLayout` |
| `#E2A15A` | DRAW: UI draw | After Clay, overlays, and optional ImGui |
| `#A6A6B0` | GRAPH: profile graph | After graph drawing, or near-zero when hidden |
| `#E5D569` | SWAP: presentation | After FPS accounting and `SDL_GL_SwapWindow` |

Every zone has its own color. The bar stacks zones in fixed execution order,
bottom to top. The legend ranks all 28 zones by total time in the rolling
100-frame history: `01` is the largest bottleneck, and entries can move as
their costs change. The five-second stderr table independently ranks zones
by time in that reporting interval. Its rank can differ from the live legend.
The full key and current hex color for all zones appear in the stderr report.
To recolor a zone, change its row in `FrameProfileGraph::Zones`, or call
`FrameProfileGraph::setZoneColor(zone, 0xRRGGBB)` at runtime.

CLR, AUR, and BG replace the former combined BG zone. CLR includes GL state
reset and the color/depth clear; AUR includes aurora vibe update and the sky
draw; BG includes world shader setup and just the selected biome, plus water
for biomes that use it. The former 8.4% BG estimate covered all three, so
the new rows have no separate prior estimate.

Every five seconds, stderr prints each zone's color, estimate where available,
**measured** share of loop work, and average milliseconds per frame, highest
time first. The estimates come from the last three active-game reports before
the latest batching changes; newly split zones have no defensible prior
estimate. Logging continues when the graph is hidden with F3. The header
records how many frames had the Extra particles setting on and the average
wall time between loop calls.

`FrameProfileGraph::mark` closes the named zone and starts the next. Its
timestamps use `SDL_GetPerformanceCounter` for submillisecond resolution.
`endFrame` writes one row into the 100-frame ring buffer. The displayed graph
is one frame behind so that the SWAP segment can include the swap.

These are CPU wall times. OpenGL calls may queue work on the GPU, and a driver
may block in a later call or in `SDL_GL_SwapWindow`. The graph cannot assign
GPU execution time to individual rendering sections. A tall SWAP segment may
include presentation or synchronization wait. The F3 switch also changes the
GRAPH zone because it skips graph drawing.

The Settings window has an **Extra particles** checkbox matching the tracker
cell editor controls for A/B profiling.
Turning it off skips snow, ball trails, spin rings, sparks, lane dust, and
confetti draws. It leaves collectables, decals, and gameplay logic active.
The stderr header reports how many frames had the checkbox on during each
five-second sample; compare samples with all frames on or all frames off.

FBO includes ball shop, bot, house, campaign detail, oil map, and story portrait
previews, plus minigame decal labels. Ball shop previews run only while the
shop is open; the carousel keeps its indices when closed. With no preview or
minigame decal needed, FBO should be near zero. Its prior 6.6% estimate was
measured before this gate and is a baseline for comparison.

World coins and gems are batched by mesh and transparency pass. All visible
lane pins, including frozen and electrified variants, and up to three live
guard pins use one compact instance upload and one draw. Each lane pin appears
once in the batch; its tint is carried in its instance data. Electrical veins
remain a separate effect pass. The pin and bot avatar zones are timed
separately so the next measurement can distinguish those costs. The shared mesh renderer also caches
the matrix, atlas, and tint uniform locations it uses on common draw paths.
The hot atlas update leaves OpenGL error checking available behind
`VTX_GL_CHECK_HOT_PATH` so routine frames avoid that driver query.
