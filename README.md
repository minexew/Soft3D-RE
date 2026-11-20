# Soft3D engine archaeology

- Engine written in C++, at least some builds include RTTI

## Game decompilation and reconstruction

[`Reversed_Game/`](Reversed_Game/) contains a self-contained C++17/CMake decompilation and reconstruction of the engine, together with an SDL3 application, platform backends, reference resources, and a test suite. It currently runs the startup and complete menu flow, but episode-specific gameplay remains unfinished. See [`Reversed_Game/README.md`](Reversed_Game/README.md) for standalone build, run, test, configuration, and library-integration instructions.

## File formats overview

|  | File extension | Platform(s) | Notes & open questions |
|--|----------------|-------------|------------------------|
|✔️|SPK|Chinese and English variants|resource archives; 64-byte and 32-byte path layouts documented in `Reverse_Docs/filepak_api.md`
|✔️|S3DTEX|S2D SDK samples|S2D images/textures; recovered direct/keyed and paletted RGB332/444/555/565/666/888 decoding, header flags, color keys, and all 173 samples documented and tested in `Reverse_Docs/s3dtex_format.md`
|✔️|S3DSTY||BSP lightmap-frame animation sequences; 256-record layout, loader, update cadence, and lightmap modulation documented and tested in `Reverse_Docs/s3dsty_format.md`
|🔧|SAI||<p>3D model animations, mostly understood</p><ul><li>Fixed-point transform tracks, bounds, morph targets, timing, and interpolation documented in `Reverse_Docs/soft3d_models_animations.md`</li></ul>
|✔️|SAU|DA320 (all), GA330, S60v1 (chi), S60v2, S60v3, Win32 (chi)|AMI ADPCM audio, low nibble first; same format as WAR except nibble order, well-understood
|🔧|SBN|GA330, S60v3|font (glyphs) + text data (32-bit encoding); Symbian base/local glyphs, string tables, character lookup, and drawing documented in `Reverse_Docs/sbn_format.md`
|🔧|SBN|Win32|font (glyphs) + text data (7-bit encoding), mostly understood
|🔧|SBP|GA330, S60v3|<p>maps, based on [Quake 2 map format](https://www.flipcode.com/archives/Quake_2_BSP_File_Format.shtml), mostly understood</p><ul><li>Corrected chunk order, all chunk readers, incremental loading, PVS visibility, fixed-point rendering, lightmaps, and map-model submission documented in `Reverse_Docs/sbp_format.md`; portable real-map loading/rendering, BSP collision, path queries, and generic unit integration are tested</li></ul>
|🔧|SDT|S60v3|Various data tables; UI resource names, all seven generic dialog tables, concrete menu classes, interpolation, secondary textures, blitter behavior, and the 1,252-byte `config.sdt` layout documented in `Reverse_Docs/sdt_format.md`; `itemdata`, all seven `gameitem` files, all five `einfo` files, and fixed game resource tables documented and tested in `Reverse_Docs/game_data_tables.md`; license-specific `message.sdt`/`domain.sdt` consumers remain stage 14 work
|✔️|SPL||Counted degree-N Bezier camera paths; fixed-point evaluation, derivatives, chord-weighted timing, camera integration, and real-resource tests documented in `Reverse_Docs/spl_format.md`
|🔧|SOJ||<p>3D objects, mostly understood</p><ul><li>Materials, packed texture names, vertex streams, primitive modes, bounds, and skinning links documented in `Reverse_Docs/soft3d_models_animations.md`</li><li>Renderer-facing material state, raster behavior, and Object loading/control APIs are documented and tested in `Reverse_Docs/symbian3rd_raster.md` and `Reverse_Docs/symbian3rd_effects.md`</li></ul>
|🔧|SST|S60v3, Win32|“simple script”; Mobile typed records, references, recursive commands, VM, tasks, and generic host interface documented and tested in `Reverse_Docs/sst_format.md`; `GameSI`'s episode-specific command semantics remain separate
|✔️|STX||images/textures (indexed/direct RGB565 and palette variants), well-understood
|✔️|WAR|DA320 (old), S60v1 (eng), Win32 (eng)|AMI ADPCM audio, high nibble first; same format as SAU except nibble order, well-understood

_Let us know if you have had success parsing files from platforms not listed in the table!_

## Some links

### 7 Days

- [Table of known versions of the game](http://web.archive.org/web/20200710065823/https://vitawiki.xyz/doku.php?id=7ds:versions)
- http://legendsworld.net/adventure/game/2455
- http://www.legendsworld.net/phpBB3/viewtopic.php?f=5&t=9745
- https://imgur.com/a/3IMWR7r
- https://sourceforge.net/projects/spkextractor/

### Misc

- [Domestic "Silent Hill" IMGA award-winning mobile game-"7 Nights", can not afford the award](http://web.archive.org/web/20200709200829/http://www.1b2g.com/newsyx/1246.html) (story of the demise of Dingoo Games, in Chinese)

## Games known to use the engine

- 7 Days: Salvation
- [天地道 aka Hellstriker II](https://www.google.com/search?q=天地道+dingoo&tbm=isch) (ships on GA330)
- 斗地主 (ships on GA330)

## Platforms

- Dingoo A320
- Gemei A330
- Symbian S60
- Win32
