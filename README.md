# Soft3D engine archaeology

- Engine written in C++, at least some builds include RTTI

## File formats overview

|  | File extension | Platform(s) | Notes & open questions |
|--|----------------|-------------|------------------------|
|✔️|SPK|Chinese and English variants|resource archives; 64-byte and 32-byte path layouts documented in `Reverse_Docs/filepak_api.md`
|🔧|S3DTEX||S2D images/textures; observed grayscale8/RGB565 decoding and remaining header/engine-loader questions documented in `Reverse_Docs/s3dtex_format.md`
|🔧|S3DSTY||BSP lightmap-frame animation sequences; 256-record layout and loader documented in `Reverse_Docs/s3dsty_format.md`; runtime timing/modulation remains under reconstruction
|🔧|SAI||<p>3D model animations, mostly understood</p><ul><li>Fixed-point transform tracks, bounds, morph targets, timing, and interpolation documented in `Reverse_Docs/soft3d_models_animations.md`</li></ul>
|✔️|SAU|DA320 (all), GA330, S60v1 (chi), S60v2, S60v3, Win32 (chi)|AMI ADPCM audio, low nibble first; same format as WAR except nibble order, well-understood
|🔧|SBN|GA330, S60v3|font (glyphs) + text data (32-bit encoding); Symbian base/local glyphs, string tables, character lookup, and drawing documented in `Reverse_Docs/sbn_format.md`
|🔧|SBN|Win32|font (glyphs) + text data (7-bit encoding), mostly understood
|🔧|SBP|GA330, S60v3|<p>maps, based on [Quake 2 map format](https://www.flipcode.com/archives/Quake_2_BSP_File_Format.shtml), mostly understood</p><ul><li>Corrected chunk order, serialized records, all chunk readers, incremental loader, map models/entities, and remaining rendering/visibility work documented in `Reverse_Docs/sbp_format.md`; portable real-map loading is tested</li></ul>
|🔧|SDT|S60v3|Various data tables; UI string/resource and dialog-state formats documented in `Reverse_Docs/sdt_format.md`; other SDT families remain unconfirmed
|🔧|SPL||Counted Bezier camera-path records; container/layout and remaining evaluation work documented in `Reverse_Docs/spl_format.md`
|🔧|SOJ||<p>3D objects, mostly understood</p><ul><li>Materials, packed texture names, vertex streams, primitive modes, bounds, and skinning links documented in `Reverse_Docs/soft3d_models_animations.md`</li><li>Some material and renderer-state fields remain unidentified</li></ul>
|🔧|SST||“simple script”, mostly understood
|✔️|STX||images/textures (RGB444), well-understood
|✔️|STX||images/textures (RGB565), well-understood
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
