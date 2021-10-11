#include "vgui_StoreMainwin.h"

class CStoreSellPanel : public CStorePanel
{
public:
	vector<containeritem_t> m_SelectedItems;

	bool InterestedInItem( msstring_ref pszItemName );
	void SellAll( );

	CStoreSellPanel( Panel *pParent );

	//Item callbacks
	void ItemCreated( void *pData );
	void ItemSelectChanged( ulong ID, bool fSelected );
	void ItemHighlighted( void *pData );
};
