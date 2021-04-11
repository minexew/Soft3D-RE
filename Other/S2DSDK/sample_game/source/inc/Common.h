
/* ================================================================
 * 版权所有 2006-2009 成都丁果科技有限公司，保留所有权利。
 * 
 * 允许对代码和二进制文件的使用、改动、再发布，但必须满足以下条件：
 * 
 * 1. 再发布的源代码，必须保留完整的版权信息。
 * 
 * 2. 再发布的二进制文件，必须产生完整的版权信息。
 * 
 * 3. 再发布版本中包括的最终用户文档，必须包括以下信息：
 * “本产品包括丁果科技所开发的软件内容。”
 * 或者，本信息可以出现在软件本身，例如第三方的声明信息中。
 *
 * 4. 使用本软件的产品，不得使用“Soft2D”或“S2D”作为产品名称，也不得在
 * 产品名称中包括“Soft2D”或“S2D”的字样，除非事先获得丁果科技的书面许可。
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL DINGOO GAMES OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 */ 

#ifndef __COMMON_H__
#define __COMMON_H__


#include "s2d/s2d.h"

using namespace s2d;

#define A320	1

struct SingleAni
{
	Texture*	m_pTex;
	s32			m_iOffsetX;
	s32			m_iOffsetY;
};
struct AniData
{
	s32		m_nFramCnt;
	SingleAni* m_pData;
};
enum Difficulty
{
	Easy = 0,
	Normal,
	Hard,
};
enum BubbleState
{
	BS_NotUse = 0,
	BS_Normal,
	BS_Moving,
	BS_CanPop,
	BS_Poping,
	BS_CanDrop,
	BS_CanNotDrop,
	BS_Droping,
	BS_Failed,
};
enum BubbleDir
{
	BD_Left = 0,
	BD_Right,
	BD_LeftTop,
	BD_RightTop,
	BD_LeftBottom,
	BD_RightBottom,
	MAX_BallDir
};
enum SceneState
{
	SS_MainMenu,
	SS_Diff,
	SS_Help,
	SS_Menu,
	SS_Contral,
	SS_Cale,
	SS_CreateNew,
	SS_Pop,
	SS_Failed,
	SS_HightScore,
};
enum AuidoTab
{
	Auido_bgmusic = 0,
	Auido_rotate,
	Auido_cale,
	Auido_pop,
	Auido_tracewall,
	Auido_tracebubble,
	Auido_savebubble,
	Auido_newscore,
	Auido_failed,
	Auido_click,
	Auido_warning,
	Auido_clear,
	Auido_cannotcontrol,
	Auido_mainui,
	MAX_AUIDO
};

enum AniTab
{
	Animation_uimainui = 0,	
	Animation_uititlebase,
	Animation_uititle,
	Animation_uilogobase,
	Animation_uilogo,
	Animation_uibubble1,
	Animation_uibubble2,
	Animation_uibubble3,
	Animation_uihightscore,
	Animation_uihelp,
	Animation_uidiffsel,
	Animation_uimenu,
	Animation_caletower,
	Animation_dragonnormal,
	Animation_dragonbusy,
	Animation_clear,
	Animation_warning1,
	Animation_warning2,
	Animation_warning3,
	Animation_warning4,
	Animation_warning5,
	Animation_uncontrol,
	Animation_faided,
	MAX_ANIMATION
};
#endif//__COMMON_H__