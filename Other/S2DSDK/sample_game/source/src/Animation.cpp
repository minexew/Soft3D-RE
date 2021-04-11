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

#include "../inc/Animation.h"
//--------------------------------------------------------------------------------------------------------
Animation::Animation(void) : m_pData(NULL),m_iCurFrame(0),m_bLoop(0)
{

}
//--------------------------------------------------------------------------------------------------------
Animation::~Animation(void)
{

}
void	Animation::Draw( Draw2D* pDraw2D, s32 x, s32 y  )
{
	assert( m_pData && m_iCurFrame >= 0 && m_iCurFrame<m_pData->m_nFramCnt );
	SingleAni* pAni = &m_pData->m_pData[m_iCurFrame];
	
	x +=pAni->m_iOffsetX;
	y +=pAni->m_iOffsetY;

	/// 居中画
	x -= pAni->m_pTex->GetWidth() >> 1;
	y -= pAni->m_pTex->GetHeight() >> 1;
	
	pDraw2D->SetTexture( pAni->m_pTex );
	pDraw2D->DrawOverlay( x, y );

}

void	Animation::DrawLF( Draw2D*	pDraw2D, s32 x, s32 y )
{

	assert( m_pData && m_pData->m_nFramCnt>0 );
	SingleAni* pAni;
	for( s32 i=0; i<m_pData->m_nFramCnt; i++ ){
		pAni = &m_pData->m_pData[m_iCurFrame];

		if( pAni->m_pTex ){
			pDraw2D->SetTexture( pAni->m_pTex );
			pDraw2D->DrawOverlay( x+pAni->m_iOffsetX, y+pAni->m_iOffsetY );
		}
	}
}

void	Animation::Exec( void )
{
	m_iCurFrame++;
	if( m_iCurFrame>=m_pData->m_nFramCnt ){
		if( m_bLoop ){
			m_iCurFrame = 0;
		}else{
			m_iCurFrame = m_pData->m_nFramCnt-1;
		}
	}
}

bool	Animation::IsAnimOver( void )
{
	return !m_bLoop && m_iCurFrame == m_pData->m_nFramCnt-1;
}

void	Animation::Reset( void )
{
	//m_pData = NULL;
	
	m_iCurFrame = 0;
}

void	Animation::Clone( Animation& anim )
{
	m_pData =  anim.m_pData;
	
	m_iCurFrame = anim.m_iCurFrame;

	m_bLoop =  anim.m_bLoop;
}