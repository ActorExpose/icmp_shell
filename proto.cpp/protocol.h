
typedef unsigned short uint16;
typedef unsigned char uint8;
typedef unsigned int uint32;

typedef struct _PacketHdr
{
	uint16 UnZipLen;  //δѹ��ǰ�ĳ���
	uint16 ZipLen; //ѹ����ĳ���
    uint32 Seq;//���ı�� 
}PacketHdr;

typedef void (CALLBACK *pf_callback)(uint8 * pData,uint32 uSize);

class CpacketHandler
{
public:
	CpacketHandler(pf_callback func)
	{

	}
	~CpacketHandler()
	{

	}

	BOOL ProcessPacket(PacketHdr *hdr);

private:


};

//.,.... ����  ��ʱ���ٸ���  �ܲ����ˡ�������


