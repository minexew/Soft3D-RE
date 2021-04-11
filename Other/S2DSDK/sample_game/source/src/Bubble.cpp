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


#include "../inc/Bubble.h"
#include "../inc/Scene.h"
#include "../inc/GameEngine.h"
Bubble::Bubble() :m_iColor(-1),
m_eState(BS_NotUse),
m_iX(I2FP(0)),
m_iY(I2FP(0)),
m_iSpeedX(I2FP(0)),
m_iSpeedY(I2FP(0)),
m_pCurAni(NULL)
{
}
Bubble::~Bubble()
{
}
bool Bubble::Init( Scene* pScene )
{
	m_pScene = pScene;
	m_AniNormal.m_bLoop = true;
	m_AniPop.m_bLoop = false;
	m_AniFailed.m_bLoop = true;
	m_AniNormal.m_pData = &m_pScene->m_BubbeAnim[ 0 ];
	m_AniPop.m_pData = &m_pScene->m_BubbePopAnim[ 0 ];
	m_AniFailed.m_pData = &m_pScene->m_BubbeFailedAnim[ 0 ];
	return true;
}

void		Bubble::Clone( Bubble& bubbe )
{
	
	m_eState =  bubbe.m_eState;

	m_iColor = bubbe.m_iColor;
	m_AniNormal.Clone( bubbe.m_AniNormal );
	m_AniPop.Clone( bubbe.m_AniPop );
	m_AniFailed.Clone( bubbe.m_AniFailed );
	
	switch( m_eState )
	{
	case BS_Droping:
	case BS_Normal:
		m_pCurAni = &m_AniNormal;
		break;
	case BS_Poping:
		m_pCurAni = &m_AniPop;
		break;
	case BS_Failed:
		m_pCurAni = &m_AniFailed;
		break;
	}
	m_iX = bubbe.m_iX;
	m_iY = bubbe.m_iY;
	m_tmpX = bubbe.m_tmpX;
	m_tmpY = bubbe.m_tmpY;
}

void		Bubble::Reset( void )
{
	m_iColor = 0;
	m_eState = BS_NotUse;
	m_iX = I2FP(0);
	m_iY = I2FP(0);
	m_iSpeedX = I2FP(0);
	m_iSpeedY = I2FP(0);
}
/// 生成
void		Bubble::Star( s32 color, f32 x, f32 y  )
{
	if( color >= m_pScene->m_iMaxColorCnt ){
		m_eState = BS_NotUse;
		return;
	}
	m_iColor = color;
	m_iX = x;
	m_iY = y;
	m_AniNormal.Reset();
	m_AniPop.Reset();
	m_AniFailed.Reset();
	m_AniNormal.m_pData = &m_pScene->m_BubbeAnim[ color ];
	m_AniPop.m_pData = &m_pScene->m_BubbePopAnim[ color ];
	m_AniFailed.m_pData = &m_pScene->m_BubbeFailedAnim[ color ];
	m_pCurAni = &m_AniNormal;
	m_eState = BS_Normal;
}
void		Bubble::Draw( Draw2D*	pDraw2D )
{
	if( m_eState == BS_NotUse ){
		return;
	}
	m_pCurAni->Draw( pDraw2D, FP2I(m_iX), FP2I(m_iY) );
}
void		Bubble::Exec( void )
{
	switch( m_eState )
	{
	case BS_Normal:
		NormalExe();
		break;
	case BS_Moving:
		MoveExe();
		break;
	case BS_Poping:
		PopExe();
		break;
	case BS_Droping:
		DropExe();
		break;
	}
}
/// 
void		Bubble::NormalExe( void )
{
	if( m_pCurAni == NULL ){
		m_pCurAni =  &m_AniNormal;
	}
	m_pCurAni->Exec();
}

/// 破碎中
void		Bubble::PopExe( void )
{
	if( m_pCurAni == NULL ){
		m_pCurAni =  &m_AniPop;
	}
	m_pCurAni->Exec();
	if( m_pCurAni->IsAnimOver() )
	{
		m_eState = BS_NotUse;
	}
}

/// 掉落中
void		Bubble::DropExe( void )
{
	if( m_pCurAni == NULL ){
		m_pCurAni =  &m_AniNormal;
	}
	m_pCurAni->Exec();
	m_tmpX = m_iX + m_iSpeedX;
	m_tmpY = m_iY + m_iSpeedY;
	
	if(  m_pScene->TraceBottom(this) )
	{
		m_eState = BS_NotUse;
		return;
	}
	m_iX = m_tmpX;
	m_iY = m_tmpY;
}

///移动中
void		Bubble::MoveExe( void )
{
	if( m_pCurAni == NULL ){
		m_pCurAni =  &m_AniNormal;
	}
	m_pCurAni->Exec();

	m_tmpX = m_iX + m_iSpeedX;
	m_tmpY = m_iY + m_iSpeedY;
	
	//和其他球的碰撞
	if( m_pScene->TraceBubble( m_tmpX, m_tmpY, m_iX, m_iY ) )
	{
		m_pScene->BubbleStop( this );
		m_pScene->m_pEng->PlaySound( Auido_tracebubble );
		return;
	}
	s32 ref = m_pScene->TraceWall( m_tmpX, m_tmpY, m_iX, m_iY, m_iSpeedX, m_iSpeedY );
	if( ref )
	{
		if( ref == 1 ){
			m_pScene->BubbleStop( this );
		}
		m_pScene->m_pEng->PlaySound( Auido_tracewall );
		return;
	}
	m_iX = m_tmpX;
	m_iY = m_tmpY;
}
/// 破碎
void		Bubble::Pop( void )
{
	m_eState = BS_Poping;
	m_pCurAni = &m_AniPop;
}

/// 掉落
void		Bubble::Drop( void )
{
	m_eState = BS_Droping;
	m_iSpeedX = I2FP(0);
	m_iSpeedY = F2FP(10);
	m_pCurAni = &m_AniNormal;
}

///移动
void		Bubble::Move( f32 x, f32 y )
{
	m_iSpeedX = x;
	m_iSpeedY = y;
	m_eState = BS_Moving;
}
///停止移动
void		Bubble::Stop( void )
{
	m_eState = BS_Normal;
}
/// 和球碰撞
bool		Bubble::Trace( f32 x, f32 y, f32 &x2, f32 &y2 )
{
	s32 ix1 = FP2I(x);
	s32 iy1 = FP2I(y);
	s32 ix2 = FP2I(m_iX);
	s32 iy2 = FP2I(m_iY);
	s32 idia = FP2I(m_pScene->m_fDiameter*F2FP(0.95));
	bool btrace = (idia*idia)>(ix1-ix2)*(ix1-ix2)+(iy1-iy2)*(iy1-iy2);

	if( btrace )
	{
		f32 temp = y2 - m_iY;
		temp.data = Abs(temp.data );
		if( temp < m_pScene->m_fRadius ){
			if( x2> m_iX ){
				x2 = m_iX + m_pScene->m_fDiameter;
			}else{
				x2 = m_iX - m_pScene->m_fDiameter;
			}
			y2 = m_iY;
			return btrace;
		}
		s32 ang =atan( (x2 - m_iX )/(y2 - m_iY) );
		x2 = m_iX + m_pScene->m_fDiameter*sin( ang );
		y2 = m_iY + m_pScene->m_fDiameter*cos( ang );
	}

	return btrace;
}
