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


#ifndef __SCENE_H__
#define __SCENE_H__

//--------------------------------------------------------------------------------------------------------
#include "Common.h"
#include "Bubble.h"

class GameEngine;
//--------------------------------------------------------------------------------------------------------
class Scene
{
public:
	Scene( void );
	~Scene( void );

public:
	/// init scene
	bool		Init( Input* pInput );

	/// init scene
	void		Reset();
	
	/// free scene
	void		Free( void );

	/// Draw scene
	void		Draw( Draw2D* pDraw2d );

	/// update scene;
	void		Exec( void );

	// 获取输入
	Input*		GetInput( void );

	/// 读入动画
	void		LoadAnim( const Char* name, AniData& data );

	/// 新游戏
	void			NewGame( Difficulty diff );

	/// 游戏结束
	void			EndGame();

	/// 游戏失败
	void			FaidedGame();

	/// 随机生成一行
	void			CreateLine( s32 r );

	/// 画选择菜单
	void			UpDataMenu( );

	/// 画选择菜单
	void			DrawMenu( Draw2D* pDraw2d );
	
	/// 爆破泡泡
	void			PopBubble( s32 c, s32 r );

	/// 计算可以爆破的泡泡数量
	s32				CountSamePop( s32 c, s32 r, s32 color);
	
	// 掉落悬空的泡泡
	void			DropBubble( void );

	/// 标记会掉落的泡泡
	bool			MarkDrop( s32 c, s32 r );

	/// 发射泡泡
	void			CaleShoot();

	///修改发射角度
	void		ControlAngles( void );

	/// 根据行列号，得到泡泡屏幕坐标
	void		GetBubbleCenter( s32 c, s32 r, f32& x, f32& y );	

	/// 根据屏幕坐标，得到泡泡行列号
	void		GetBubblePos( s32& c, s32& r, f32 x, f32 y );	

	/// 产生随机数
	s32			Rand( s32 min, s32 max );
	
	/// 和墙碰撞
	s32			TraceWall( f32 x, f32 y, f32 &x2, f32 &y2, f32& speedx, f32& speedy );

	/// 和其他泡泡碰撞
	bool		TraceBubble( f32 x, f32 y, f32 &x2, f32 &y2 );
	
	/// 和底部碰撞
	bool		TraceBottom( Bubble* pBubble );

	/// 弹射泡泡停止
	void		BubbleStop( Bubble* pBubble );

	/// 画发射线
	void		DrawCaleLine( Draw2D* pDraw2d );

	/// 弹射新的泡泡上去
	bool		CaleNewBubble( void );

	/// 保存泡泡
	void		SaveBubble( void );

	/// 增加难度
	void		Levelup( void );

	/// 读取高分榜
	void		ReadHightScore();

	/// 写高分榜
	void		WriteHightScore(s32 HigSc);

	/// 画数字
	void		DrawDigital( Draw2D* pDraw2d, s32 digital, s32 x, s32 y, s32 type );

	/// 读入配置文件
	void		ReadConfig( void );
private:

	//--------------------------------------------------------------------------------------------------------
	//### data members:
public:
	
			/// 最大颜色数量
    enum{
		m_iMaxColorCnt = 7,
	};
	/// 当前状态
	SceneState			m_eState;
	SceneState			m_eStateTemp;
	

	/// scene rect
	Rect		m_SceneRect;

	/// 当前分数
	s32				m_iScore;

	/// 当前等级
	s32				m_iLevel;

	/// 新增加的分数
	s32				m_iAddScore;

	///增加分数位置
	s32				m_iAddScX;
	s32				m_iAddScY;
	s32				m_iAddCnt;

	/// 泡泡半径
	f32			m_fRadius;

	/// 泡泡直径
	f32			m_fDiameter;

	/// 难度
	Difficulty		m_eDifficulty;

	/// 行数增长时间
	s32				m_iGrowTime;
	s32				m_iCntGrow;

	/// 游戏中UI
	s32			m_bShowUI;

	s32			m_UICurSele;
	
	// 列数
	s32			m_nOfColumns;
	// 行数
	s32			m_nOfRows;

	/// 奇偶行标志
	s32			m_level;
	
	/// 泡泡数量
	s32			m_nOfBubble;
	///泡泡
	Bubble*		m_pBubbleBuffer;
	
	/// 等待发射的泡泡
	Bubble		m_CaleBubble;

	/// 预备泡泡
	Bubble		m_ReadyBubble[2];

	/// 弹射新的泡泡
	//Bubble		m_NewBubble;
	s32			m_iCaleCnt;
	s32			m_iCntNewBubble;

	/// 存储的泡泡
	Bubble		m_SaveBubble;

	// 泡泡位置
	AniData		m_BubblePalce;
	
	/// 当前准备发射角度
	s32			m_iAngles;

	/// 泡泡发射的速度
	f32			m_fCaleSpeed;
	
	// 发射线
	Texture*	m_pTexCaleLine;

	/// 发射等待时间
	s32			m_iWaitTime;

	/// 发射的泡泡停靠的位置
	s32			m_newBubleC;
	s32			m_newBubleR;

	/// 当前颜色数量
	s32			m_iCurColorCnt;
	

	/// 动画资源列表
	AniData		m_BubbeAnim[ m_iMaxColorCnt ];
	AniData		m_BubbePopAnim[ m_iMaxColorCnt ];
	AniData		m_BubbeFailedAnim[ m_iMaxColorCnt ];
	AniData		m_AniTable[ MAX_ANIMATION ];

	/// 动画列表
	Animation	m_AnimationTable[ MAX_ANIMATION ];

	/// 是否全消除
	s32			m_bAllClear;
	
	// 背景
	Texture*	m_pTexBack;
	Texture*	m_pTexLogo;
	Texture*	m_pTexHS;

	/// input
	Input*		m_pInput;

	///
	GameEngine* m_pEng;
	
	RandomGenerator	m_rand;
	
	/// 高分榜
	s32				m_iCntHS;
	s32				m_iNewScoreIdex;
	s32*			m_iHightScore;
	s32				m_iFramCnt1;

	// 数字贴图
	Texture*		m_pDigital[4];

};

#endif//__SCENE_H__
