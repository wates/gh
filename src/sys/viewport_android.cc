
#ifndef WINDOWS

#include "gfx/viewport.h"
#include "core/system.h"
#include "core/pipe.h"
#include "core/converter.h"

namespace wts{
namespace gfx{

	void ViewportMessage::Mouse(int x,int y,int z,unsigned int button)
	{
        UNUSED(x);
        UNUSED(y);
        UNUSED(z);
        UNUSED(button);
	}

	class ViewportBody
		:public Viewport
	{
	public:
		void Open(const InitializeParameter &ip,ViewportMessage *cb)
		{
            UNUSED(ip);
			count_=0;
			cb_=cb;
			cb->Open();
		}
		void Close()
		{
			cb_->Close();
		}
		void*GetWindowHandle()
		{
			return NULL;
		}
		void MessageFetch()
		{
			count_++;
			if(100==count_)
			{
				cb_->CloseButton();
			}
		}
        int Width()const
        {
            return width_;
        }
        int Height()const
        {
            return height_;
        }
        void SetSize(int w,int h)
        {
            width_=w;
            height_=h;
        }
		ViewportBody()
            :count_(0)
		{
		}
		~ViewportBody()
		{
		}
	private:
        int width_;
        int height_;
		int count_;
		ViewportMessage *cb_;
	};

	Viewport* AppendViewport()
	{
		ViewportBody* vp=new ViewportBody;
		return vp;
	}

	void DeleteViewport(Viewport *p)
	{
		ViewportBody* vp=static_cast<ViewportBody*>(p);
		delete vp;
	}

}
}


#endif
