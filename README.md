# EvolveArt — evolutionary image builder for Geometry Dash

Recreates any image inside a GD level, live, using the same idea as the
"Generating Videos in Geometry Dash with Evolution" video: generate a batch
of random candidate objects, keep whichever one most reduces the difference
between the level and the target image, repeat forever. Quality improves the
longer you let it run.

## How it's organized

| File | What it does | Confidence |
|---|---|---|
| `src/EvolutionEngine.hpp/.cpp` | The actual evolutionary algorithm. Pure C++, no GD dependency — loads/downsamples the target image, simulates a tiny internal canvas, scores random candidates, keeps improvements. | High — this is plain, testable logic. |
| `src/EvolvePopup.hpp/.cpp` | In-editor popup: pick an image, Start/Pause/Stop, live progress. Runs the engine on a scheduler tick so it doesn't freeze the game. | High — standard Geode UI patterns. |
| `src/LevelBuilder.hpp/.cpp` | Turns each accepted "gene" into a real placed object + colour in your level. | **Needs a quick check** — see below. |
| `src/main.cpp` | Adds an "Evolve" button to the editor that opens the popup. | High. |

## Before you build

1. Install the [Geode CLI](https://docs.geode-sdk.org/getting-started/) and
   set the `GEODE_SDK` environment variable to your SDK checkout (the CLI
   does this for you if you use `geode new` — you can drop this project's
   `src/` files into a fresh `geode new` project if you'd rather start from
   a guaranteed-current template).
2. Open `src/LevelBuilder.cpp`. Two lines are marked `NOTE: verify` —
   they set a colour channel's live colour and assign an object to a colour
   channel. I'm confident about the overall approach (GD colours are
   channel-based, not per-object) and about object ID `1` being the basic
   square block (checked against the community object-ID reference at
   a-zalt.github.io/gdknowledge/resources/object_ids.html), but the exact
   method names on `GJEffectManager`/`GameObject` can differ by a word or
   two between Geode/GD versions. If either line doesn't compile:
   - Open `Geode/binding/GJEffectManager.hpp` and `Geode/binding/GameObject.hpp`
     in your Geode SDK checkout (or browse them at docs.geode-sdk.org/classes).
   - Look for the method that (a) sets a colour channel's RGB, and (b) sets
     which channel a `GameObject` points to. Swap in the correct names —
     nothing else in the file needs to change.
   - If you get stuck, the Geode Discord's #help channel is very fast for
     "what's this binding called now" questions.
   `geode::utils::file::pick` in `EvolvePopup.cpp` follows the shape used by
   most current Geode mods for an open-file dialog, but if the compiler
   complains about `FilePickOptions`/`Filter`/`.listen(...)`, check
   `Geode/utils/file.hpp` for the current signature — it's a small,
   mechanical fix.
3. `geode build` (or build through the CLI's usual workflow) and install
   the resulting `.geode` file, or use `geode dev` to hot-reload while
   testing.

## Using it

1. Open any level in the editor.
2. Click the new "Evolve" button (top-left).
3. Choose Image → pick a PNG/JPG.
4. Press Start. Objects will start appearing to the right of your spawn
   point and the image will sharpen over time — leave it running as long as
   you like; it gets closer to the source image the longer it runs, and
   stops on its own once it hits the object budget in the mod's settings.
5. Pause/Stop any time. Adjust population size, generations per tick, canvas
   resolution, and object budget in the mod's settings page — bigger canvas
   and population = sharper image but slower and heavier on the editor.

## Known v0.1 limitations (on purpose, to keep this buildable)

- By default the builder places square objects only (object ID 1). There is now a setting to allow the engine to choose any object ID from 1 to 120, which can create much richer visual detail when your version of GD supports those object types.
- Colour uses a 216-colour palette (a 6×6×6 RGB cube) rather than GD's
  per-object HSV-copy trick, which is how the original video got smoother
  gradients from a single channel. The palette approach is simpler to get
  right first, but if you want to chase that extra fidelity later, the
  hook point is `LevelBuilder::channelForColor` / `setupColorPalette`.
- No undo — evolving into an existing level's objects will happen if you
  don't leave empty space, so start from a blank layer/section.
- Video mode is now supported. Use the new "Choose Frames" button, pick a
  folder of image files, and the mod will generate each frame in turn along
  the X axis with a configurable empty gap between frames.

## Credit

Concept based on the evolutionary image-recreation technique from
"Generating Videos in Geometry Dash with Evolution".
