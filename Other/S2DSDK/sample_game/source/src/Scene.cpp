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
#include "../inc/Scene.h"
#include "../inc/GameEngine.h"
//--------------------------------------------------------------------------------------------------------
Scene::Scene(void)
{

}
//--------------------------------------------------------------------------------------------------------
Scene::~Scene(void)
{

}
//--------------------------------------------------------------------------------------------------------
/// init scene
bool		Scene::Init( Input* pInput )
{
	s32 i;
	m_pInput = pInput;
	m_rand.srand( GetCurTime() );

	m_eState = SS_MainMenu;
	m_eStateTemp = SS_MainMenu;

	m_eDifficulty = Easy;

	// 列数
	m_nOfColumns = 8;
	// 行数
	m_nOfRows = 10;

	ReadConfig();

	/// 奇偶行标志
	m_level = 0;

	m_iCntNewBubble = 0;
	m_iScore = 0;
	m_iHightScore = NULL;
	m_iCntHS = 0;
	m_iNewScoreIdex = -1;
	m_iWaitTime = 144;
	m_iFramCnt1 = 0;
	m_iAddScX = 0;
	m_iAddScY = 0;
	m_iAddCnt = 0;
	m_bAllClear = 0;
	m_iLevel = 0;
	
	/// 泡泡数量
	m_nOfBubble = m_nOfColumns*m_nOfRows;

	///泡泡
	m_pBubbleBuffer = new Bubble[m_nOfBubble];
	for(  i=0; i<m_nOfBubble; i++ )
	{
		m_pBubbleBuffer[i].Init( this );
	}
	m_CaleBubble.Init( this );
	m_ReadyBubble[0].Init( this );
	m_ReadyBubble[1].Init( this );
	//m_NewBubble.Init( this );
	m_SaveBubble.Init( this );

	/// 当前颜色数量
	m_iCurColorCnt = 0;
	
	LoadAnim( _LS("bubbepop1.txt"), m_BubbeAnim[0] );
	LoadAnim( _LS("bubbepop2.txt"), m_BubbeAnim[1] );
	LoadAnim( _LS("bubbepop3.txt"), m_BubbeAnim[2] );
	LoadAnim( _LS("bubbepop4.txt"), m_BubbeAnim[3] );
	LoadAnim( _LS("bubbepop5.txt"), m_BubbeAnim[4] );
	LoadAnim( _LS("bubbepop6.txt"), m_BubbeAnim[5] );
	LoadAnim( _LS("bubbepop7.txt"), m_BubbeAnim[6] );

	LoadAnim( _LS("bubbepopa.txt"), m_BubbePopAnim[0] );
	LoadAnim( _LS("bubbepopb.txt"), m_BubbePopAnim[1] );
	LoadAnim( _LS("bubbepopc.txt"), m_BubbePopAnim[2] );
	LoadAnim( _LS("bubbepopd.txt"), m_BubbePopAnim[3] );
	LoadAnim( _LS("bubbepope.txt"), m_BubbePopAnim[4] );
	LoadAnim( _LS("bubbepopf.txt"), m_BubbePopAnim[5] );
	LoadAnim( _LS("bubbepopg.txt"), m_BubbePopAnim[6] );

	LoadAnim( _LS("bubbepop1.txt"), m_BubbeFailedAnim[0] );
	LoadAnim( _LS("bubbepop2.txt"), m_BubbeFailedAnim[1] );
	LoadAnim( _LS("bubbepop1.txt"), m_BubbeFailedAnim[2] );
	LoadAnim( _LS("bubbepop1.txt"), m_BubbeFailedAnim[3] );
	LoadAnim( _LS("bubbepop5.txt"), m_BubbeFailedAnim[4] );
	LoadAnim( _LS("bubbepop6.txt"), m_BubbeFailedAnim[5] );
	LoadAnim( _LS("bubbepop7.txt"), m_BubbeFailedAnim[6] );

	LoadAnim( _LS("bubbe.txt"), m_BubblePalce );

	LoadAnim( _LS("lose.txt"), m_AniTable[Animation_faided] );	
	LoadAnim( _LS("ui_01.txt"),  m_AniTable[Animation_uimainui] );
	LoadAnim( _LS("ui_02.txt"), m_AniTable[Animation_uidiffsel] );
	LoadAnim( _LS("ui_03.txt"), m_AniTable[Animation_uimenu] );
	LoadAnim( _LS("ui_help.txt"), m_AniTable[Animation_uihelp] );
	LoadAnim( _LS("ui_phb.txt"), m_AniTable[Animation_uihightscore] );
	LoadAnim( _LS("dise.txt"), m_AniTable[Animation_uititlebase] );
	LoadAnim( _LS("bash.txt"), m_AniTable[Animation_uititle] );
	LoadAnim( _LS("popo.txt"), m_AniTable[Animation_uilogobase] );
	LoadAnim( _LS("paopao.txt"), m_AniTable[Animation_uilogo] );
	LoadAnim( _LS("qiu_01.txt"), m_AniTable[Animation_uibubble1] );
	LoadAnim( _LS("qiu_02.txt"), m_AniTable[Animation_uibubble2] );
	LoadAnim( _LS("qiu_03.txt"), m_AniTable[Animation_uibubble3] );
	LoadAnim( _LS("jingshi_05.txt"), m_AniTable[Animation_warning1] );
	LoadAnim( _LS("jingshi_04.txt"), m_AniTable[Animation_warning2] );
	LoadAnim( _LS("jingshi_03.txt"), m_AniTable[Animation_warning3] );
	LoadAnim( _LS("jingshi_02.txt"), m_AniTable[Animation_warning4] );
	LoadAnim( _LS("jingshi_01.txt"), m_AniTable[Animation_warning5] );
	LoadAnim( _LS("good.txt"), m_AniTable[Animation_clear] );
	LoadAnim( _LS("shikong.txt"), m_AniTable[Animation_uncontrol] );
	LoadAnim( _LS("long_01.txt"), m_AniTable[Animation_dragonnormal] );
	LoadAnim( _LS("long_02.txt"), m_AniTable[Animation_dragonbusy] );
	LoadAnim( _LS("cale.txt"), m_AniTable[Animation_caletower] );	

	for(  i=0; i<MAX_ANIMATION; i++ ){
		m_AnimationTable[i].Reset();
		m_AnimationTable[i].m_pData = &m_AniTable[i];
	}
	m_AnimationTable[Animation_uncontrol].m_bLoop = true;
	m_AnimationTable[Animation_dragonnormal].m_bLoop = true;
	m_AnimationTable[Animation_dragonbusy].m_bLoop = true;
	
	m_iCntHS = m_AniTable[Animation_uihightscore].m_nFramCnt/3;
	m_iHightScore = (s32*)malloc( 4*m_iCntHS );
	memset( (Char*)m_iHightScore, 0, 4*m_iCntHS );

	m_pTexBack = m_pEng->GetTexture( _LS("bj") );
	m_pTexCaleLine =  m_pEng->GetTexture( _LS("dian") );
	m_pTexLogo = m_pEng->GetTexture( _LS("logo") );
	m_pTexHS =  m_pEng->GetTexture( _LS("phb") );
	m_pDigital[0] = m_pEng->GetTexture( _LS("shuzi_01") );
	m_pDigital[1] = m_pEng->GetTexture( _LS("shuzi_02") );
	m_pDigital[2] = m_pEng->GetTexture( _LS("shuzi_03") );
	m_pDigital[3] = m_pEng->GetTexture( _LS("shuzi_04") );

	//NewGame( Easy );
	ReadHightScore();

	return true;
}
//--------------------------------------------------------------------------------------------------------
/// init scene
void		Scene::Reset()
{
	for( s32 i=0; i<m_nOfBubble; i++ )
	{
		m_pBubbleBuffer[i].Reset();
	}
}
//--------------------------------------------------------------------------------------------------------
/// free scene
void		Scene::Free( void )
{
	s32 i;
	SAFE_DELETE_ARRAY( m_pBubbleBuffer );
	for(  i=0; i< m_iMaxColorCnt; i++ ){
		SAFE_FREE( m_BubbeAnim[i].m_pData );
		SAFE_FREE( m_BubbePopAnim[i].m_pData );
		SAFE_FREE( m_BubbeFailedAnim[i].m_pData );
	}
	for(  i=0; i< MAX_ANIMATION; i++ ){
		SAFE_FREE(m_AniTable[i].m_pData );
	}
	SAFE_FREE(m_BubblePalce.m_pData );
	SAFE_FREE(m_iHightScore );
}

//--------------------------------------------------------------------------------------------------------
/// 读入所有动画
void		Scene::LoadAnim( const Char* name, AniData& data )
{
	void* fp = GetDLRes( name );

	if(fp == NULL){
		return;
	}
	s32 size = GetDLResSize( fp );
	Char* buffer = (Char*)malloc( size );
	memset( (Char*)buffer, 0, size );
	ReadDLResData( fp, buffer, size, 1 ); 
	CloseDLRes( fp );

	Char* pSrc = buffer;
	

	sscanf( pSrc, _LS("%d"), &data.m_nFramCnt );

	Char cur = *pSrc;
	while( cur != 0x0A && cur!=0 && pSrc-buffer<size-1 ) {
		cur = *pSrc++;
	}

	data.m_pData = (SingleAni*)malloc( sizeof( SingleAni ) *data.m_nFramCnt  );
	Char texname[256];
	for( s32 i=0; i<data.m_nFramCnt; i++ )
	{
		sscanf( pSrc, _LS("%s,%d,%d"), texname, &data.m_pData[i].m_iOffsetX,  &data.m_pData[i].m_iOffsetY );
		data.m_pData[i].m_pTex = m_pEng->GetTexture( texname );
		
		cur=*pSrc;
		while( cur != 0x0A && cur!=0 && pSrc-buffer<size-1 ) {
			cur = *pSrc++;
		}
	}
	SAFE_FREE( buffer );
}
//--------------------------------------------------------------------------------------------------------
/// Draw scene
void		Scene::Draw( Draw2D* pDraw2d )
{
	switch( m_eState )
	{
	case SS_MainMenu:
		{
			pDraw2d->SetTexture( m_pTexLogo );
			pDraw2d->DrawOverlay( 0, 0 );

			m_AnimationTable[Animation_uimainui].DrawLF( pDraw2d, 0, 0 );	

			m_AnimationTable[Animation_uititlebase].DrawLF( pDraw2d, 0, 0 );
			m_AnimationTable[Animation_uititle].DrawLF( pDraw2d, 0, 0 );
			m_AnimationTable[Animation_uilogobase].DrawLF( pDraw2d, 0, 0 );
			m_AnimationTable[Animation_uilogo].DrawLF( pDraw2d, 0, 0 );
			m_AnimationTable[Animation_uibubble1].DrawLF( pDraw2d, 0, 0 );
			m_AnimationTable[Animation_uibubble2].DrawLF( pDraw2d, 0, 0 );
			m_AnimationTable[Animation_uibubble3].DrawLF( pDraw2d, 0, 0 );
		}
		return;
	case SS_Help:
		{
			m_AnimationTable[Animation_uihelp].DrawLF( pDraw2d, 0, 0 );
		}
		return;
	case SS_Diff:
		{
			pDraw2d->SetTexture( m_pTexLogo );
			pDraw2d->DrawOverlay( 0, 0 );
			m_AnimationTable[Animation_uidiffsel].DrawLF( pDraw2d, 0, 0 );
				
			m_AnimationTable[Animation_uititlebase].DrawLF( pDraw2d, 0, 0 );
			m_AnimationTable[Animation_uititle].DrawLF( pDraw2d, 0, 0 );
			m_AnimationTable[Animation_uilogobase].DrawLF( pDraw2d, 0, 0 );
			m_AnimationTable[Animation_uilogo].DrawLF( pDraw2d, 0, 0 );
			m_AnimationTable[Animation_uibubble1].DrawLF( pDraw2d, 0, 0 );
			m_AnimationTable[Animation_uibubble2].DrawLF( pDraw2d, 0, 0 );
			m_AnimationTable[Animation_uibubble3].DrawLF( pDraw2d, 0, 0 );
		}
		return;
	case SS_HightScore:
		{
			pDraw2d->SetTexture( m_pTexHS );
			pDraw2d->DrawOverlay( 0, 0 );
			m_iFramCnt1++;
			if( m_iNewScoreIdex>=0 && m_iNewScoreIdex<m_iCntHS && (m_iFramCnt1%16)>8 ){
				m_AnimationTable[Animation_uihightscore].m_iCurFrame = m_iNewScoreIdex;
				m_AnimationTable[Animation_uihightscore].DrawLF( pDraw2d, 0, 0 );
			}
			for( s32 i=0;i<m_iCntHS; i++ ){
				DrawDigital( pDraw2d, i+1, m_AniTable[Animation_uihightscore].m_pData[i+8].m_iOffsetX, m_AniTable[Animation_uihightscore].m_pData[i+8].m_iOffsetY,  3 );
				DrawDigital( pDraw2d, m_iHightScore[i], m_AniTable[Animation_uihightscore].m_pData[i+16].m_iOffsetX, m_AniTable[Animation_uihightscore].m_pData[i+16].m_iOffsetY,  1 );
			}
		}
		return;
	}

	pDraw2d->Clear(0,0,0);
	pDraw2d->SetTexture( m_pTexBack );
	pDraw2d->DrawOverlay( 0, 0 );
	m_AnimationTable[Animation_caletower].Draw( pDraw2d, 0, 0 );
	
	// 指示线
	DrawCaleLine( pDraw2d );
	// 发射塔
	m_CaleBubble.Draw( pDraw2d);
	m_ReadyBubble[0].Draw(pDraw2d);
	m_ReadyBubble[1].Draw(pDraw2d);

	for( s32 i=0; i<m_nOfBubble; i++ )
	{
		m_pBubbleBuffer[i].Draw( pDraw2d );
	}
	
	s32 sce = m_iCntGrow / 24 ;
	if(sce < 5 && sce >= 0 ){
		m_AnimationTable[Animation_warning1+sce].DrawLF(pDraw2d,0,0);
	}
	//m_NewBubble.Draw( pDraw2d );
	m_SaveBubble.Draw( pDraw2d );
	if( m_eState == SS_Failed )
	{
		m_AnimationTable[Animation_faided].Draw( pDraw2d,0,0);
	}
	if( m_eState == SS_Menu )
	{
		m_AnimationTable[Animation_uimenu].DrawLF( pDraw2d, 0, 0 );
	}
	if( m_iAddCnt>0 ){
		m_iAddCnt--;
		DrawDigital( pDraw2d, m_iAddScore, m_iAddScX, m_iAddScY, 0 );
		m_iAddScY-=2;
	}
	/// 分数
	DrawDigital( pDraw2d, m_iScore, m_BubblePalce.m_pData[4].m_iOffsetX, m_BubblePalce.m_pData[4].m_iOffsetY, 1 );

	///全清特效
	if( m_bAllClear ){
		m_AnimationTable[Animation_clear].DrawLF(pDraw2d,0,0);
	}
	/// 失去控制
	if( SS_CreateNew == m_eState ){
		m_AnimationTable[Animation_dragonbusy].DrawLF( pDraw2d, 0, 0 );
		m_AnimationTable[Animation_uncontrol].DrawLF( pDraw2d, 0, 0 );
	}else{
		m_AnimationTable[Animation_dragonnormal].DrawLF( pDraw2d, 0, 0 );
	}
	
}
//--------------------------------------------------------------------------------------------------------
/// update scene;
void		Scene::Exec( void )
{
	switch( m_eState )
	{
	case SS_MainMenu:
		{
	
			m_AnimationTable[Animation_uititlebase].Exec();
			m_AnimationTable[Animation_uititle].Exec();
			m_AnimationTable[Animation_uilogobase].Exec();
			m_AnimationTable[Animation_uilogo].Exec();
			m_AnimationTable[Animation_uibubble1].Exec();
			m_AnimationTable[Animation_uibubble2].Exec();
			m_AnimationTable[Animation_uibubble3].Exec();
			if( m_pInput->IsKeyClick(KEY_UP) ){
				m_AnimationTable[Animation_uimainui].m_iCurFrame--;
				if( m_AnimationTable[Animation_uimainui].m_iCurFrame<0 ){
					m_AnimationTable[Animation_uimainui].m_iCurFrame = m_AniTable[Animation_uimainui].m_nFramCnt-1;
				}
				m_pEng->PlaySound( Auido_click );
			}
			if( m_pInput->IsKeyClick(KEY_DN) ){
				m_AnimationTable[Animation_uimainui].m_iCurFrame++;
				if( m_AnimationTable[Animation_uimainui].m_iCurFrame>=m_AniTable[Animation_uimainui].m_nFramCnt ){
					m_AnimationTable[Animation_uimainui].m_iCurFrame = 0;
				}
				m_pEng->PlaySound( Auido_click );
			}
			if( m_pInput->IsKeyClick(KEY_OK) ){
				if(m_AnimationTable[Animation_uimainui].m_iCurFrame == 0){
					m_eState = SS_Diff;
				}
				if(m_AnimationTable[Animation_uimainui].m_iCurFrame == 1){
					m_eState = SS_HightScore;
				}
				if(m_AnimationTable[Animation_uimainui].m_iCurFrame == 2){
					m_eState = SS_Help;
				}
				if(m_AnimationTable[Animation_uimainui].m_iCurFrame == 3){
					EndGame();
				}
				m_pEng->PlaySound( Auido_click );
			}
		}
		return;
	case SS_Help:
		{			
			if( m_pInput->IsKeyClick(KEY_LF)  ){
				m_AnimationTable[Animation_uihelp].m_iCurFrame--;
				if( m_AnimationTable[Animation_uihelp].m_iCurFrame<0 ){
					m_AnimationTable[Animation_uihelp].m_iCurFrame = m_AniTable[Animation_uihelp].m_nFramCnt-1;
				}
				m_pEng->PlaySound( Auido_click );
			}
			if( m_pInput->IsKeyClick(KEY_RT ) ){
				m_AnimationTable[Animation_uihelp].m_iCurFrame++;
				if( m_AnimationTable[Animation_uihelp].m_iCurFrame>=m_AniTable[Animation_uihelp].m_nFramCnt ){
					m_AnimationTable[Animation_uihelp].m_iCurFrame = 0;
				}
				m_pEng->PlaySound( Auido_click );
			}
			if( m_pInput->IsKeyClick(KEY_R1) ){
				m_eState = SS_MainMenu;
				m_pEng->PlaySound( Auido_click );
			}
		}
		return;
	case SS_Diff:
		{
			m_AnimationTable[Animation_uititlebase].Exec();
			m_AnimationTable[Animation_uititle].Exec();
			m_AnimationTable[Animation_uilogobase].Exec();
			m_AnimationTable[Animation_uilogo].Exec(); 
			m_AnimationTable[Animation_uibubble1].Exec();
			m_AnimationTable[Animation_uibubble2].Exec();
			m_AnimationTable[Animation_uibubble3].Exec();
			if( m_pInput->IsKeyClick(KEY_UP) ){
				m_AnimationTable[Animation_uidiffsel].m_iCurFrame--;
				if( m_AnimationTable[Animation_uidiffsel].m_iCurFrame<0 ){
					m_AnimationTable[Animation_uidiffsel].m_iCurFrame = m_AniTable[Animation_uidiffsel].m_nFramCnt-1;
				}
				m_pEng->PlaySound( Auido_click );
			}
			if( m_pInput->IsKeyClick(KEY_DN) ){
				m_AnimationTable[Animation_uidiffsel].m_iCurFrame++;
				if( m_AnimationTable[Animation_uidiffsel].m_iCurFrame>=m_AniTable[Animation_uidiffsel].m_nFramCnt ){
					m_AnimationTable[Animation_uidiffsel].m_iCurFrame = 0;
				}
				m_pEng->PlaySound( Auido_click );
			}
			if( m_pInput->IsKeyClick(KEY_OK) ){
				NewGame( (Difficulty)m_AnimationTable[Animation_uidiffsel].m_iCurFrame );
				m_pEng->PlaySound( Auido_click );
			}
			if( m_pInput->IsKeyClick(KEY_R1) ){
				m_eState = SS_MainMenu;
				m_pEng->PlaySound( Auido_click );
			}
		}
		return;
	case SS_Menu:
		{
			if( m_pInput->IsKeyClick(KEY_UP) ){
				m_AnimationTable[Animation_uimenu].m_iCurFrame--;
				if( m_AnimationTable[Animation_uimenu].m_iCurFrame<0 ){
					m_AnimationTable[Animation_uimenu].m_iCurFrame = m_AniTable[Animation_uimenu].m_nFramCnt-1;
				}
				m_pEng->PlaySound( Auido_click );
			}
			if( m_pInput->IsKeyClick(KEY_DN) ){
				m_AnimationTable[Animation_uimenu].m_iCurFrame++;
				if( m_AnimationTable[Animation_uimenu].m_iCurFrame>=m_AniTable[Animation_uimenu].m_nFramCnt ){
					m_AnimationTable[Animation_uimenu].m_iCurFrame = 0;
				}
				m_pEng->PlaySound( Auido_click );
			}
			if( m_pInput->IsKeyClick(KEY_OK) ){
				if( m_AnimationTable[Animation_uimenu].m_iCurFrame == 0 ){
					m_eState = m_eStateTemp;
				}else if( m_AnimationTable[Animation_uimenu].m_iCurFrame == 1 ){
					m_pEng->PlayMuisc( Auido_mainui, true );
					m_eState = SS_MainMenu;
				}else{
					EndGame();
				}
				m_pEng->PlaySound( Auido_click );
			}
			if( m_pInput->IsKeyClick(KEY_R1) ){
				m_eState = m_eStateTemp;
			}
		}
		return;
	case SS_HightScore:
		{
			if( m_pInput->IsKeyClick(KEY_R1) ){
				if( m_iNewScoreIdex >= 0){
					m_iNewScoreIdex = -1;
				}
				m_eState = SS_MainMenu;
				m_eStateTemp = SS_MainMenu;
				m_pEng->PlayMuisc( Auido_mainui, true );
				m_pEng->PlaySound( Auido_click );
			}
		}
		return;
	}
	m_eStateTemp = m_eState;
	if( m_pInput->IsKeyClick(KEY_R1) ){
		m_eState = SS_Menu;
	}
	ControlAngles();
	
	m_CaleBubble.Exec();
	m_ReadyBubble[0].Exec();
	m_ReadyBubble[1].Exec();
	//m_NewBubble.Exec();
	m_SaveBubble.Exec();

	for( s32 i=0; i<m_nOfBubble; i++ )
	{
		m_pBubbleBuffer[i].Exec();
	}

	m_iCntGrow--;
	s32 sce = m_iCntGrow / 24 ;
	if(sce < 5 && sce >= 0 ){
		m_AnimationTable[Animation_warning1+sce].Exec();
	}
	Levelup();

	switch( m_eState )
	{
	case SS_Contral:
		{
			if( m_iCntGrow==m_iCntGrow/24*24 && m_iCntGrow<=120 ){
				m_pEng->PlaySound( Auido_warning );
			}
			if( m_iCntGrow < 0 && m_CaleBubble.m_eState==BS_Normal ){
				m_iCaleCnt = m_iCntNewBubble;
				m_iCntGrow = m_iGrowTime;
				m_eState = SS_CreateNew;
				m_pEng->PlaySound( Auido_cannotcontrol );
				return;
			}

			if( m_pInput->IsKeyClick( KEY_OK ) ){
				CaleShoot();
			}
		}
		break;
	case SS_Cale:
		{
			if( m_CaleBubble.m_eState == BS_Normal ){
				m_eState = SS_Contral;
			}
		}
		break;
	case SS_CreateNew:
		{
			if( !CaleNewBubble() ){
				m_AnimationTable[Animation_warning1].Reset();
				m_AnimationTable[Animation_warning2].Reset();
				m_AnimationTable[Animation_warning3].Reset();
				m_AnimationTable[Animation_warning4].Reset();
				m_AnimationTable[Animation_warning5].Reset();
				m_eState = SS_Contral;
			}
		}
		break;
	case SS_Pop:
		{
			for( s32 i=0; i<m_nOfBubble; i++ )
			{
				if( m_pBubbleBuffer[i].m_eState != BS_NotUse &&  m_pBubbleBuffer[i].m_eState != BS_Normal ){
					m_eState = SS_Contral;
				}
			}
		}
		break;
	}

	if( m_eState == SS_Failed )
	{
		m_AnimationTable[Animation_faided].Exec();
		if( m_AnimationTable[Animation_faided].IsAnimOver() ){
			WriteHightScore( m_iScore );
		}
	}

	///全清特效
	if( m_bAllClear ){
		if( m_AnimationTable[Animation_clear].IsAnimOver() ){
			m_bAllClear = 0;
		}
		m_AnimationTable[Animation_clear].Exec();
	}

	/// 龙的动画
	m_AnimationTable[Animation_dragonnormal].Exec();
	m_AnimationTable[Animation_dragonbusy].Exec();
	/// 等待一帧后，爆破泡泡，便于玩家看清楚状况
	//if( m_newBubleC >=0 && m_newBubleR>=0 ){
	//	m_newBubleC = -1;
	//	m_newBubleR = -1;
	//	PopBubble( m_newBubleC, m_newBubleR );
	//}
}
//--------------------------------------------------------------------------------------------------------
/// 新游戏
void			Scene::NewGame( Difficulty diff )
{
	m_eDifficulty = diff;
	m_iAngles = 90;
	m_newBubleC = -1;
	m_newBubleR = -1;
	m_AnimationTable[Animation_caletower].m_iCurFrame = m_AniTable[Animation_caletower].m_nFramCnt/2;
	m_iCaleCnt = 0;
	m_eState = SS_Contral;
	m_SaveBubble.m_eState = BS_NotUse;
	m_iScore = 0;
	m_AnimationTable[Animation_faided].Reset();
	m_AnimationTable[Animation_clear].Reset();
	m_bAllClear = 0;

	for( s32 i=0; i<m_nOfBubble; i++ )
	{
		m_pBubbleBuffer[i].m_eState = BS_NotUse;
	}
	
	if( m_eDifficulty == Easy )
	{
		m_iCurColorCnt = 3;
		m_iCntNewBubble = 3;
		m_iLevel = 0;
		CreateLine(0);
		CreateLine(1);
	}
	else if(  m_eDifficulty == Normal )
	{
		m_iCurColorCnt = 4;
		m_iCntNewBubble = 4;
		m_iLevel = 1;
		CreateLine(0);
		CreateLine(1);
		CreateLine(2);
	}
	else
	{
		m_iCurColorCnt = 5;
		m_iCntNewBubble = 5;
		m_iLevel = 3;
		CreateLine(0);
		CreateLine(1);
		CreateLine(2);
		CreateLine(3);
	}
	m_CaleBubble.Star( Rand( 0, m_iCurColorCnt ), I2FP(m_BubblePalce.m_pData[0].m_iOffsetX),  I2FP(m_BubblePalce.m_pData[0].m_iOffsetY) );
	m_ReadyBubble[0].Star( Rand( 0, m_iCurColorCnt ), I2FP(m_BubblePalce.m_pData[1].m_iOffsetX),  I2FP(m_BubblePalce.m_pData[1].m_iOffsetY) );
	m_ReadyBubble[1].Star( Rand( 0, m_iCurColorCnt ), I2FP(m_BubblePalce.m_pData[2].m_iOffsetX),  I2FP(m_BubblePalce.m_pData[2].m_iOffsetY) );
	m_iCntGrow = m_iGrowTime;
	m_pEng->PlayMuisc(Auido_bgmusic,true );
}
//--------------------------------------------------------------------------------------------------------
/// 游戏结束
void			Scene::EndGame()
{
	Exit(0);
}
//--------------------------------------------------------------------------------------------------------
/// 游戏失败
void			Scene::FaidedGame()
{
	/// 
	m_eState = SS_Failed;
	m_pEng->PlaySound( Auido_failed );
	m_pEng->StopMuisc();
}
/// 和墙碰撞
s32		Scene::TraceWall( f32 x, f32 y,f32 &x2, f32 &y2, f32& speedx, f32& speedy )
{
	Rect bubblerect;
	bubblerect.left = FP2I( x - m_fRadius );
	bubblerect.right = FP2I( x + m_fRadius );
	bubblerect.top = FP2I( y - m_fRadius );
	bubblerect.bottom = FP2I( y + m_fRadius );
	
	if( m_SceneRect.top > bubblerect.top ){
		//到达最高处
		
		f32 tan =(x - x2 )/(y - y2);
		y2 = I2FP(m_SceneRect.top) + m_fRadius;
		x2 = x2 - m_fRadius * tan;

		return 1;
	}
	if( m_SceneRect.left > bubblerect.left ){
		speedx *= I2FP(-1);
		return 2;
	}
	if( m_SceneRect.right < bubblerect.right ){
		speedx *= I2FP(-1);
		return 2;
	}

	return 0;
}

/// 和其他泡泡碰撞
bool		Scene::TraceBubble( f32 x, f32 y, f32 &x2, f32 &y2 )
{
	for( s32 i=0; i<m_nOfBubble; i++ )
	{
		if( m_pBubbleBuffer[i].m_eState == BS_Normal ){
			if( m_pBubbleBuffer[i].Trace( x, y, x2, y2 )){
				//m_pEng->PlaySound( Auido_tracebubble );
				return true;
			}
		}
	}
	return false;
}
//--------------------------------------------------------------------------------------------------------
/// 和底部碰撞
bool		Scene::TraceBottom( Bubble* pBubble )
{
	s32 bottom = FP2I( pBubble->m_tmpY + m_fRadius );
	
	if( m_SceneRect.bottom <bottom ){
		return true;
	}
	return false;
}
//--------------------------------------------------------------------------------------------------------
/// 发射泡泡
void	Scene::CaleShoot()
{
	m_eState = SS_Cale;
	f32 speedx = m_fCaleSpeed * cosf( Deg2Ang(m_iAngles) );
	f32 speedy = -m_fCaleSpeed * sinf( Deg2Ang(m_iAngles) );
	m_CaleBubble.Move( speedx,speedy );
	m_pEng->PlaySound( Auido_cale );
	// 播放一次按压的动画
	//m_AnimationTable[Animation_dragonnormal].m_iCurFrame = 0;
}
///修改发射角度
void	Scene::ControlAngles( void )
{
	if( m_eState == SS_CreateNew ){
		return;
	}
	if( m_pInput->IsKeyDown( KEY_RT ) || m_pInput->IsKeyPress( KEY_RT )  ){
		m_iAngles -= 4;
		if( m_iAngles<30){
			m_iAngles = 30;
		}
		m_pEng->PlaySound( Auido_rotate );
		m_AnimationTable[Animation_caletower].m_iCurFrame = (m_iAngles-30)/(120/(m_AniTable[Animation_caletower].m_nFramCnt-1)) ;
	}
	if( m_pInput->IsKeyDown( KEY_LF ) || m_pInput->IsKeyPress( KEY_LF )  ){
		m_iAngles += 4;
		if( m_iAngles>150){
			m_iAngles = 150;
		}
		m_pEng->PlaySound( Auido_rotate );
		m_AnimationTable[Animation_caletower].m_iCurFrame = (m_iAngles-30)/(120/(m_AniTable[Animation_caletower].m_nFramCnt-1)) ;
	}
	if( m_pInput->IsKeyClick(KEY_L1) ){
		SaveBubble();
	}
}
//--------------------------------------------------------------------------------------------------------
/// 弹射泡泡停止
void		Scene::BubbleStop( Bubble* pBubble )
{
	s32 c, r, index;
	GetBubblePos( c, r, pBubble->m_iX, pBubble->m_iY );
	if( c<0 )c=0;
	if( c>m_nOfColumns-1- r%2)c=m_nOfColumns-1 - r%2;
	index = r*m_nOfColumns+c;

	f32 x, y;
	GetBubbleCenter( c, r, x,  y ); 
	m_pBubbleBuffer[ index ].Star( pBubble->m_iColor, x, y );

	//if( pBubble == &m_CaleBubble ){
	pBubble->Clone( m_ReadyBubble[0] );
	pBubble->m_iX =  I2FP(m_BubblePalce.m_pData[0].m_iOffsetX);
	pBubble->m_iY =  I2FP(m_BubblePalce.m_pData[0].m_iOffsetY);
	m_ReadyBubble[0].Clone( m_ReadyBubble[1] );
	m_ReadyBubble[0].m_iX =  I2FP(m_BubblePalce.m_pData[1].m_iOffsetX);
	m_ReadyBubble[0].m_iY =  I2FP(m_BubblePalce.m_pData[1].m_iOffsetY);
	m_ReadyBubble[1].Star( Rand( 0, m_iCurColorCnt ), 
		I2FP(m_BubblePalce.m_pData[2].m_iOffsetX),  
		I2FP(m_BubblePalce.m_pData[2].m_iOffsetY) );

	//m_newBubleC = c;
	//m_newBubleR = r;
	if( SS_CreateNew != m_eState ){
		PopBubble( c,r );
	}
	//}else{
	//	pBubble->m_eState = BS_NotUse;
	//}
	if( r == m_nOfRows-1 && m_pBubbleBuffer[r*m_nOfColumns+c].m_eState == BS_Normal ){
		FaidedGame();
	}
}
//--------------------------------------------------------------------------------------------------------
/// 画发射线
void		Scene::DrawCaleLine( Draw2D* pDraw2d )
{
	if( m_eState == SS_CreateNew ){
		return;
	}
	s32 cont = 0;
	if( m_eDifficulty == Easy ){
		cont= 20;
	}else if( m_eDifficulty == Normal ){
		cont= 10;
	}
	f32 x = I2FP(m_BubblePalce.m_pData[0].m_iOffsetX);
	f32 y = I2FP(m_BubblePalce.m_pData[0].m_iOffsetY);
	
	f32 speedx = m_fRadius * cosf( Deg2Ang(m_iAngles) );
	f32 speedy = -m_fRadius * sinf( Deg2Ang(m_iAngles) );

	s32 ix, iy;
	f32	tx = x, ty = y;
	for( s32 i=0; i<cont; i++ ){
		tx += speedx;
		ty += speedy;
		if( TraceWall( tx, ty, x, y, speedx, speedy ) == 1 || TraceBubble( x-speedx, y-speedy, x, y )){
			return;
		}
		x += speedx;
		y += speedy;
		pDraw2d->SetTexture( m_pTexCaleLine );
		ix = FP2I(x) - (m_pTexCaleLine->GetWidth() >> 1);
		iy = FP2I(y) - (m_pTexCaleLine->GetHeight() >> 1);
		pDraw2d->DrawOverlay( ix, iy );
	}
}
//--------------------------------------------------------------------------------------------------------
/// 弹射新的泡泡上去
bool		Scene::CaleNewBubble( void )
{
	m_AnimationTable[Animation_uncontrol].Exec();
	if( m_CaleBubble.m_eState != BS_Normal ){
		return true;
	}
	if( m_iCaleCnt <= 0 ){
		return false;
	}
	m_iCaleCnt--;
	f32 x =  I2FP(m_BubblePalce.m_pData[0].m_iOffsetX);
	f32 y =  I2FP(m_BubblePalce.m_pData[0].m_iOffsetY);
	m_iAngles = Rand( 30, 150 );
	m_CaleBubble.Move( F2FP(15) * cosf( Deg2Ang(m_iAngles) ), F2FP(-15) * sinf( Deg2Ang(m_iAngles) ) );
	m_AnimationTable[Animation_caletower].m_iCurFrame = (m_iAngles-30)/(120/(m_AniTable[Animation_caletower].m_nFramCnt-1)) ;
	return true;
}
//--------------------------------------------------------------------------------------------------------
/// 保存泡泡
void		Scene::SaveBubble( void )
{
	if( m_CaleBubble.m_eState != BS_Normal ){
		return;
	}
	m_pEng->PlaySound( Auido_savebubble );
	if( m_SaveBubble.m_eState == BS_NotUse )
	{
		m_SaveBubble.Star( m_CaleBubble.m_iColor, I2FP( m_BubblePalce.m_pData[3].m_iOffsetX),  I2FP( m_BubblePalce.m_pData[3].m_iOffsetY) );

		m_CaleBubble.Clone( m_ReadyBubble[0] );
		m_CaleBubble.m_iX =  I2FP(m_BubblePalce.m_pData[0].m_iOffsetX);
		m_CaleBubble.m_iY =  I2FP(m_BubblePalce.m_pData[0].m_iOffsetY);
		m_ReadyBubble[0].Clone( m_ReadyBubble[1] );
		m_ReadyBubble[0].m_iX =  I2FP(m_BubblePalce.m_pData[1].m_iOffsetX);
		m_ReadyBubble[0].m_iY =  I2FP(m_BubblePalce.m_pData[1].m_iOffsetY);
		m_ReadyBubble[1].Star( Rand( 0, m_iCurColorCnt ), 
			I2FP(m_BubblePalce.m_pData[2].m_iOffsetX),  
			I2FP(m_BubblePalce.m_pData[2].m_iOffsetY) );
	}else
	{
		s32 color = m_SaveBubble.m_iColor;
		m_SaveBubble.Star( m_CaleBubble.m_iColor, I2FP( m_BubblePalce.m_pData[3].m_iOffsetX),  I2FP( m_BubblePalce.m_pData[3].m_iOffsetY) );
		m_CaleBubble.Star( color, I2FP( m_BubblePalce.m_pData[0].m_iOffsetX),  I2FP( m_BubblePalce.m_pData[0].m_iOffsetY) );
	}
}
//--------------------------------------------------------------------------------------------------------
/// 新增加一行
void			Scene::CreateLine( s32 r )
{
	f32 x, y;
	// 随机新的一行
	s32 maxcol;
	s32 start = r*m_nOfColumns;
	if( r % 2 ){
		maxcol = m_nOfColumns-1;
		m_pBubbleBuffer[start+maxcol].m_eState = BS_NotUse;
	}else{
		maxcol = m_nOfColumns;
	}
	for( s32 i = 0 ; i<maxcol; i++ ){
		GetBubbleCenter( i, r, x,  y ); 
		m_pBubbleBuffer[start+i].Star( Rand( 0, m_iCurColorCnt ), x, y );
	}

	///// 游戏失败
	//if( bFaided ){
	//	FaidedGame();
	//}
}
//--------------------------------------------------------------------------------------------------------
// 获取输入
Input*		Scene::GetInput( void )
{ 
	return m_pInput;
}
//--------------------------------------------------------------------------------------------------------
	/// 爆破泡泡
void			Scene::PopBubble( s32 c, s32 r )
{
	s32 idx		= r*m_nOfColumns + c;
	if(c<0 || c>=m_nOfColumns
		|| r<0 || r>=m_nOfRows
		|| m_pBubbleBuffer==NULL
		|| m_pBubbleBuffer[idx].m_eState == BS_NotUse ){
		return;
	}
	s32 cnt = CountSamePop( c, r, m_pBubbleBuffer[idx].m_iColor );
	if( cnt < 3 )
	{
		for( s32 i=0; i<m_nOfBubble; i++ )
		{
			if( m_pBubbleBuffer[i].m_eState == BS_CanPop )
			{
				m_pBubbleBuffer[i].m_eState = BS_Normal;
			}
		}
		return;
	}
	m_iAddScore = (m_eDifficulty+1)*cnt;
	// 掉落悬空的泡泡
	DropBubble();
	
	s32 dropcnt = 0;
	/// 该掉落掉落，该爆就爆
	m_bAllClear = 1;
	for( s32 i=0; i<m_nOfBubble; i++ )
	{
		if( m_pBubbleBuffer[i].m_eState == BS_CanPop )
		{
			m_pBubbleBuffer[i].Pop();
		}
		if( m_pBubbleBuffer[i].m_eState == BS_CanDrop )
		{
			dropcnt++;
			m_pBubbleBuffer[i].Drop();
		}
		if( m_pBubbleBuffer[i].m_eState == BS_CanNotDrop )
		{
			m_pBubbleBuffer[i].m_eState = BS_Normal;
		}
		if( m_pBubbleBuffer[i].m_eState == BS_Normal ){
			m_bAllClear = 0;
		}
	}
	m_iAddScore += dropcnt*(m_eDifficulty+1)*2;
	m_iAddCnt = 16;
	m_iAddScX = FP2I( m_pBubbleBuffer[idx].m_iX );
	m_iAddScY = FP2I( m_pBubbleBuffer[idx].m_iY );
	m_iAddScore*=10;

	/// 全部清除了
	if( m_bAllClear ){
		m_iAddScore += 100;
		m_AnimationTable[Animation_clear].Reset();
	}
	m_iScore += m_iAddScore;
	m_pEng->PlaySound( Auido_newscore );
}
//--------------------------------------------------------------------------------------------------------
/// 标记爆破泡泡
s32	Scene::CountSamePop( s32 c, s32 r, s32 color )
{
	if(c<0 || c>=m_nOfColumns || r<0 || r>=m_nOfRows)
		return 0;//出界，返回
	s32 cnt = 0;
	s32 idx		= r*m_nOfColumns + c;
	
	if(m_pBubbleBuffer[idx].m_eState != BS_Normal ){
		return 0;
	}
	
	if( m_pBubbleBuffer[idx].m_iColor == color)
	{
		m_pBubbleBuffer[idx].m_eState = BS_CanPop;
		cnt = 1;
		//递归遍历周围所有的泡泡
		if( ( r+m_level )%2 )
		{
			// 奇数行,少一个泡泡
			cnt += CountSamePop(c, r+1, color);
			cnt += CountSamePop(c+1, r+1, color);
			cnt += CountSamePop(c-1, r, color);
			cnt += CountSamePop(c+1, r, color);
			cnt += CountSamePop(c, r-1, color);
			cnt += CountSamePop(c+1, r-1, color);
		}
		else
		{
			// 偶数行
			cnt += CountSamePop(c-1, r+1, color);
			cnt += CountSamePop(c, r+1, color);
			cnt += CountSamePop(c-1, r, color);
			cnt += CountSamePop(c+1, r, color);
			cnt += CountSamePop(c-1, r-1, color);
			cnt += CountSamePop(c, r-1, color);
		}
	}
	return cnt;
}
//--------------------------------------------------------------------------------------------------------
// 掉落悬空的泡泡
void Scene::DropBubble( void )
{
	s32 index = 0;
	for( s32 c=0; c<m_nOfColumns; c++ )
	{
		for( s32 r = 0; r<m_nOfRows; r++ )
		{
			index = r*m_nOfColumns + c; 
			if( m_pBubbleBuffer[index].m_eState == BS_CanPop )
			{
				/// 会爆破的泡泡，测试周围的泡泡是否会掉落
				if( ( r+m_level )%2 )
				{
					// 奇数行,少一个泡泡
					MarkDrop( c, r-1 );//左上
					MarkDrop( c+1, r-1 );//右上
					MarkDrop( c-1, r );//左
					MarkDrop( c+1, r );//右
					MarkDrop( c, r+1 );//左下
					MarkDrop( c+1, r+1 );//右下
				}else
				{
					//偶
					MarkDrop( c-1, r-1 );//左上
					MarkDrop( c, r-1 );//右上
					MarkDrop( c-1, r );//左
					MarkDrop( c+1, r );//右
					MarkDrop( c-1, r+1 );//左下
					MarkDrop( c, r+1 );//右下
				}
			}
		}
	}
}
//--------------------------------------------------------------------------------------------------------
/// 标记会掉落的泡泡
bool Scene::MarkDrop( s32 c, s32 r )
{
	if( r< 0 ){
		return true;//到顶部了，不会掉落,终止递归
	}
	if( c < 0 || c>=m_nOfColumns || r>=m_nOfRows){
		return false;//出界，返回
	}
	s32 idx		= r*m_nOfColumns + c;
	if( m_pBubbleBuffer[idx].m_eState == BS_CanNotDrop ){
		return true;//这个区域已经检查过了，不会掉落,终止递归
	}

	if( m_pBubbleBuffer[idx].m_eState != BS_Normal ){
		return false;//没有泡泡，是会爆破的泡泡
	}
	m_pBubbleBuffer[idx].m_eState = BS_CanDrop;

	if( ( r+m_level )%2 )
	{
		// 奇数行,少一个泡泡
		if( MarkDrop( c, r-1 ) )//左上
		{
			m_pBubbleBuffer[idx].m_eState = BS_CanNotDrop;
			return true;
		}

		if( MarkDrop( c+1, r-1 ) )//右上
		{
			m_pBubbleBuffer[idx].m_eState = BS_CanNotDrop;
			return true;
		}
		if( MarkDrop( c-1, r ) )//左
		{
			m_pBubbleBuffer[idx].m_eState = BS_CanNotDrop;
			return true;
		}
		if( MarkDrop( c+1, r ) )//右
		{
			m_pBubbleBuffer[idx].m_eState = BS_CanNotDrop;
			return true;
		}
		if( MarkDrop( c, r+1 ) )//左下
		{
			m_pBubbleBuffer[idx].m_eState = BS_CanNotDrop;
			return true;
		}
		if( MarkDrop( c+1, r+1 ) )//右下
		{
			m_pBubbleBuffer[idx].m_eState = BS_CanNotDrop;
			return true;
		}

	}else
	{
		if( MarkDrop( c-1, r-1 ) ) //左上
		{
			m_pBubbleBuffer[idx].m_eState = BS_CanNotDrop;
			return true;
		}
		if( MarkDrop( c, r-1 ) ) //右上
		{
			m_pBubbleBuffer[idx].m_eState = BS_CanNotDrop;
			return true;
		}
		if( MarkDrop( c-1, r ) ) //左
		{
			m_pBubbleBuffer[idx].m_eState = BS_CanNotDrop;
			return true;
		}
		if( MarkDrop( c+1, r ) ) //右
		{
			m_pBubbleBuffer[idx].m_eState = BS_CanNotDrop;
			return true;
		}
		if( MarkDrop( c-1, r+1 ) ) //左下
		{
			m_pBubbleBuffer[idx].m_eState = BS_CanNotDrop;
			return true;
		}
		if( MarkDrop( c, r+1 ) )//右下
		{
			m_pBubbleBuffer[idx].m_eState = BS_CanNotDrop;
			return true;
		}
	}
	return false;
}
//--------------------------------------------------------------------------------------------------------
/// 增加难度
void		Scene::Levelup( void )
{
	if( m_iScore > (m_iLevel+1) * 500 )
	{
		m_iLevel++;
		if( m_iLevel%2 ){
			m_iCurColorCnt++;
		}else{
			m_iCntNewBubble++;
		}
	}

	if( m_iCurColorCnt > m_iMaxColorCnt ){
		m_iCurColorCnt = m_iMaxColorCnt;
	}
	if( m_iCntNewBubble > 15 ){
		m_iCntNewBubble = 15;
	}
}
//--------------------------------------------------------------------------------------------------------
/// 根据行列号，得到泡泡屏幕坐标
void	Scene::GetBubbleCenter( s32 c, s32 r, f32& x, f32& y )
{
	x = I2FP(c) * m_fDiameter + I2FP( m_SceneRect.left ) + m_fRadius;
	y = I2FP(r) * m_fDiameter + I2FP( m_SceneRect.top ) + m_fRadius;
	
	if( ( r+m_level )%2 ){
		x += m_fRadius;
	}
}
//--------------------------------------------------------------------------------------------------------
/// 根据屏幕坐标，得到泡泡行列号
void	Scene::GetBubblePos( s32& c, s32& r, f32 x, f32 y )
{
	r = FP2I(( y - I2FP( m_SceneRect.top ) ) / m_fDiameter);
	if( ( r+m_level )%2 ){
		x -= m_fRadius;
	}
	c = FP2I(( x - I2FP( m_SceneRect.left ) ) / m_fDiameter);
}	
//--------------------------------------------------------------------------------------------------------
/// 产生随机数
s32		Scene::Rand( s32 min, s32 max )
{
	if( min >= max ){
		return max;
	}
	return ( (m_rand.rand() % ( ( max - min ) << 6 ) )>>6 ) + min;
}
//--------------------------------------------------------------------------------------------------------
/// 读取高分榜
void		Scene::ReadHightScore()
{
	Char fullname[256];
	sprintf( fullname, _LS("bubble.bin") );
	CompleteFilenameWithAppPath( fullname );
	FILE* fp = fopen( fullname, _LS("rb") );
	
	if(fp == NULL){
		return;
	}
	fread( m_iHightScore, 4, m_iCntHS, fp );
	fclose( fp );
}

//--------------------------------------------------------------------------------------------------------
/// 写高分榜
void			Scene::WriteHightScore(s32 HigSc)
{
	for(s32 i=0;i<m_iCntHS;i++){
		if(m_iHightScore[i] < HigSc)
		{
			for(s32 j=m_iCntHS-1;j>i;j--){
				m_iHightScore[j] = m_iHightScore[j-1];
			}
			m_iNewScoreIdex = i;
			m_iHightScore[i] = HigSc;
			m_eState = SS_HightScore;

			Char fullname[256];
			sprintf( fullname, _LS("bubble.bin") );
			CompleteFilenameWithAppPath( fullname );
			FILE* fp = fopen( fullname, _LS("wb") );

			if(fp == NULL){
				return;
			}
			fwrite( m_iHightScore, 4, m_iCntHS, fp );
			fclose( fp );

			return;
		}
	}
	m_eState = SS_Menu;
	return;
}
//--------------------------------------------------------------------------------------------------------
/// 画数字
void		Scene::DrawDigital( Draw2D* pDraw2d, s32 digital, s32 x, s32 y, s32 type )
{
	pDraw2d->SetTexture( m_pDigital[type] );
	s32 div = 10;
	s32 temp = digital;
	s32 width =  m_pDigital[type]->GetWidth()/10;
	Rect rect;
	rect.top = 0;
	rect.bottom =  m_pDigital[type]->GetHeight();
	while( temp ){
		rect.left = width*(temp%10);
		rect.right = rect.left+width;
		temp/=div;
		pDraw2d->DrawOverlay( x, y, &rect );
		x-=width;
	}
}
//--------------------------------------------------------------------------------------------------------
/// 读入配置文件
void		Scene::ReadConfig( void )
{
	void* fp = GetDLRes( _LS("config.txt") );
	
	if(fp == NULL){
#if	A320
		m_SceneRect.left = 40;
		m_SceneRect.top = 20;
		m_SceneRect.right = 200;
		m_SceneRect.bottom = 280;
		m_fRadius = I2FP(10);
		m_fDiameter = I2FP(20);
		m_fCaleSpeed = F2FP(10.0f);
		m_iGrowTime = 1000;
#else

#endif
		return;
	}
	s32 size = GetDLResSize( fp );
	Char* buffer = (Char*)malloc( size );
	memset( (Char*)buffer, 0, size );
	ReadDLResData( fp, buffer, size, 1 ); 
	CloseDLRes( fp );
	
	Char* pSrc = buffer;
	Char name[256];
	s32 data;
	while( *pSrc && pSrc-buffer<size-1 ){
		sscanf( pSrc, _LS("%s = %d"), name, &data );
		if( s2d::strcmp( name, _LS("left")) == 0 ){
			m_SceneRect.left = data;
		}else if( s2d::strcmp( name, _LS("right")) == 0 ){
			m_SceneRect.right = data;
		}else if( s2d::strcmp( name, _LS("top")) == 0 ){
			m_SceneRect.top = data;
		}else if( s2d::strcmp( name, _LS("bottom")) == 0 ){
			m_SceneRect.bottom = data;
		}else if( s2d::strcmp( name, _LS("radius")) == 0 ){
			m_fRadius = I2FP(data );
			m_fDiameter = I2FP(data*2);
		}else if( s2d::strcmp( name, _LS("speed")) == 0 ){
			m_fCaleSpeed = I2FP(data);
		}else if( s2d::strcmp( name, _LS("grow")) == 0 ){
			m_iGrowTime = data;
		}
		Char cur = *pSrc;
		while( cur != 0x0A && cur != 0 && pSrc-buffer<size-1 ) {
			cur = *pSrc++;
		}
	}
	m_nOfColumns = (m_SceneRect.right - m_SceneRect.left )/ FP2I(m_fDiameter);
	m_nOfRows = (m_SceneRect.bottom - m_SceneRect.top )/ FP2I(m_fDiameter);
	SAFE_FREE( buffer );
}
//--------------------------------------------------------------------------------------------------------