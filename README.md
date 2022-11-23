# Soft3D engine archaeology

- Engine written in C++, at least some builds include RTTI

## File formats overview

|  | File extension | Platform(s) | Notes & open questions |
|--|----------------|-------------|------------------------|
|✔️|S3DTEX||images/textures, well-understood
|❌|S3DSTY||
|❌|SAI||probably 3D model animations
|✔️|SAU|(all)|AMI ADPCM audio, well-understood
|🔧|SBN|GA330|<p>font (glyphs) + text data (32-bit encoding), mostly understood</p><ul><li>Why is the header length not always the same?</li><li>How to calculate glyph indexes across SBNs?</li></ul>
|🔧|SBN|Win32|font (glyphs) + text data (7-bit encoding), mostly understood
|🔧|SBP|GA330|<p>maps, based on [Quake 2 map format](https://www.flipcode.com/archives/Quake_2_BSP_File_Format.shtml), mostly understood</p><ul><li>How to interpret all the chunks?</li><li>How are lightmaps applied?</li></ul>
|❌|SDT||various _data tables_ without a common format?
|🔧|SPL||Camera settings|
|🔧|SOJ||<p>3D objects, mostly understood</p><ul><li>Skinning? Animations?</li></ul>
|🔧|SST||“simple script”, mostly understood
|✔️|STX|GA330|images/textures, well-understood
|✔️|WAR|DA320, Win32|AMI ADPCM audio, well-understood

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

- Symbian S60
- Dingoo A320
- Gemei A330
- Win32
