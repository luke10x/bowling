# Frame profile graph

The graph at the bottom of the game shows the last 100 **completed** frames,
oldest on the left. Each column is a frame; each colored segment is CPU wall
time spent between two checkpoints in `vtx::loop`. The bars normally use a
100 ms scale and clip above it; the scale expands when necessary to show the
audio deadline. The horizontal timing guides mark 16.67 ms (60 FPS),
33.33 ms (30 FPS), and 66.67 ms (15 FPS). Each guide has a black millisecond
label on the left and a black FPS label on the right; with AUDIO stacked, these
are reference budgets rather than an exact frame-rate calculation. Enable **Performance analysis** in
Settings or press **F3** to start profiling. It defaults to off. While off,
the profiler collects no samples, draws no graph, and prints no reports. The
graph sits above the 28-pixel footer, including the FPS label. The clearance
scales with display pixel ratio.

**AUDIO** is the white segment at the bottom of each frame's stacked bar. It
contains the total time in SDL audio callbacks completed since the previous
frame sample, including music, SFX, and silence paths. Most frames may have
no white segment; if several callbacks complete between frame samples, their
durations are combined in that column. AUDIO also participates in the ranked
legend and the five-second stderr table. A red horizontal guide shows the
expected callback interval in milliseconds, calculated as `1000 × obtained
buffer frames / obtained sample rate`; for example, 4096 frames at 44.1 kHz
puts `AUDIO 92.9 MS` on the graph. The guide disappears when no audio device is
open and moves after a buffer change. It is a warning threshold for the browser
main thread, not proof of an underrun: actual callback scheduling and already
buffered sound also matter. The report separately prints
callback count, average and maximum time per callback. On native builds,
audio may run concurrently with the game loop, so the stacked total is
combined CPU work rather than elapsed frame time. The white segment measures
callback execution time, not how late a callback started. Disabling
Performance analysis stops callback timing and clears its samples.

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
| `#784CB6` | FLY: flying rewards | After flying collectables below the HUD |
| `#E277A3` | CARD: block-card drop | After the block-card drop animation and optional sparks |
| `#D64E62` | DECAL: decals | After the decal batch |
| `#49B5B0` | HUD: controls and screen effects | Before Clay UI work |
| `#A9764B` | PREP: UI preparation | Before `Clay_BeginLayout` |
| `#6BA8CF` | LAY: UI layout | Before `Clay_EndLayout` |
| `#E2A15A` | DRAW: UI draw | After Clay, overlays, and optional ImGui |
| `#A6A6B0` | GRAPH: profile graph | After graph drawing |
| `#E5D569` | SWAP: presentation | After FPS accounting and `SDL_GL_SwapWindow` |
| `#FFFFFF` | AUDIO: SDL callback | Completed callback work sampled after swap |

Every zone has its own color. The bar stacks AUDIO first, then loop zones in
fixed execution order from bottom to top. The legend ranks all 30 zones by total time in the rolling
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
**measured** share of combined loop and audio work, and average milliseconds per frame, highest
time first. The estimates come from the last three active-game reports before
the latest batching changes; newly split zones have no defensible prior
estimate. Logging stops when Performance analysis is disabled. The header
records how many frames had the Extra particles setting on and the average
wall time between loop calls.

`FrameProfileGraph::mark` closes the named zone and starts the next. Its
timestamps use `SDL_GetPerformanceCounter` for submillisecond resolution.
`endFrame` writes one row into the 100-frame ring buffer. The displayed graph
is one frame behind so that the SWAP segment can include the swap.

These are CPU wall times. OpenGL calls may queue work on the GPU, and a driver
may block in a later call or in `SDL_GL_SwapWindow`. The graph cannot assign
GPU execution time to individual rendering sections. A tall SWAP segment may
include presentation or synchronization wait.

The Settings window has an **Extra particles** checkbox matching the tracker
cell editor controls for A/B profiling.
Turning it off skips snow, ambient ball-trace spawning, sparks, lane dust, and
confetti draws, plus block-card-drop spark generation and uploads. It leaves
the gameplay-important Saturn spin rings and NOS trace bursts, collectables,
decals, and gameplay logic active.
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

Flying rewards are drawn in two passes using the same HUD edge for the whole
frame. Below that edge they are translucent and draw with the world; at or
above it they draw after the HUD at full opacity. Within each pass, adjacent
rewards using the same mesh share an instance batch. This keeps the original
coin/gem/rune draw order when their paths overlap. Dedicated instance buffers
avoid restore uploads, and a lone reward keeps the single-item draw path.

DECAL keeps its 14 fixed lane-mark records on the GPU after the first draw.
Only the aiming-line record is uploaded while it changes. Its shader uniform
locations are cached, and OpenGL error polling on this hot path is available
with `VTX_GL_CHECK_HOT_PATH`.
