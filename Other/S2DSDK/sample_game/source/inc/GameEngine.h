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



#ifndef __GAMEENGINE_H__
#define __GAMEENGINE_H__

//--------------------------------------------------------------------------------------------------------
#include "Common.h"
#include "Scene.h"
#include "TextureManager.h"

//--------------------------------------------------------------------------------------------------------
class GameEngine : public Engine
{
//--------------------------------------------------------------------------------------------------------
public:
	GameEngine( void* userdata [] );
	~GameEngine( void );
//--------------------------------------------------------------------------------------------------------
//### implement Engine:
public:
	/// init all resources
	result	Init( void );

	/// free all resources
	void	Free( void );

	/// game logic before render
	void	Exec( void );

	/// game logic after render
	void	Exec2( void );

	/// pre-render proccess
	void	PreRender( void );

	/// render
	void	Render( void );

	/// post-render proccess
	void	PostRender( void );

	/// display debug info
	void	DispDebugInfo( void );

	/// define anim table
	void	DefAnimTable( void );

	/// exit game
	void		ExitGame( void );

//--------------------------------------------------------------------------------------------------------
public:
	/// 开始游戏
	void	EnterGame(s32 MapId);
	
	/// 回到主菜单
	void	ReturnManiUI();

	/// 弹出窗口
	void	PopMenu();
	
	/// 读取高分榜
	void		ReadHightScore();
	
	/// 写高分榜
	s32			WriteHightScore(s32 HigSc);

	/// 播放音乐
	void		PlayMuisc(u32 ID,bool bRep);

	/// 停止音乐
	void		StopMuisc();

	/// 播放音乐
	void		PlaySound(u32 ID);

	/// 停止音乐
	void		StopAllSound();

	/// 获取贴图
	Texture*	GetTexture( const Char* name );
//--------------------------------------------------------------------------------------------------------
//### data members:
public:
	// scene
	Scene		m_Scene;


	// shaders
	FadeShader*	m_pFadeShader;


	/// 是否开启声音
	bool			m_bUseAudio;

	///积分榜
	s32			m_iHigScore[5];
	s32			m_iScoreScene[5];
	s32			m_iNewSceneIdex;

	
	/// 声音文件
	AudioData*	m_pAudioData;
	s32			m_AudioCnt;

	LanguageType		m_iLanguage;

	TextureManager		m_TexMgr;
};
//--------------------------------------------------------------------------------------------------------

#endif//__GAMEENGINE_H__
