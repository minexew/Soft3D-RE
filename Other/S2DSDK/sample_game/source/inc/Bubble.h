
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

#ifndef __BUBBLE_H__
#define __BUBBLE_H__

//--------------------------------------------------------------------------------------------------------
#include "Common.h"
#include "Animation.h"

class Scene;
//--------------------------------------------------------------------------------------------------------
class Bubble
{
public:
	Bubble( void );
	~Bubble( void );

public:
	/// init scene
	bool		Init( Scene* pScene );

	void		Reset( void );

	void		Draw( Draw2D*	pDraw2D );

	void		Exec( void );

	void		Clone( Bubble& bubbe );

	/// 
	void		NormalExe( void );

	/// 破碎中
	void		PopExe( void );

	/// 掉落中
	void		DropExe( void );
	
	///移动中
	void		MoveExe( void );
	
	/// 生成
	void		Star( s32 color, f32 x, f32 y );

	/// 破碎
	void		Pop( void );

	/// 掉落
	void		Drop( void );
	
	///移动
	void		Move( f32 x, f32 y );

	///停止移动
	void		Stop(void);

	/// 和球碰撞
	bool		Trace( f32 x, f32 y, f32 &x2, f32 &y2 );
	
private:
	//--------------------------------------------------------------------------------------------------------
	//### data members:
	Scene*		m_pScene;
public:
	s32			m_iColor;

	BubbleState		m_eState;

	f32			m_iX;
	f32			m_iY;

	f32			m_tmpX;
	f32			m_tmpY;


	f32			m_iSpeedX;
	f32			m_iSpeedY;

	Animation*	m_pCurAni;

	Animation	m_AniNormal;
	Animation	m_AniPop;
	Animation	m_AniFailed;
};

#endif//__BUBBLE_H__
