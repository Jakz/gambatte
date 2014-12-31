//
//   Copyright (C) 2007 by sinamas <sinamas at users.sourceforge.net>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License version 2 for more details.
//
//   You should have received a copy of the GNU General Public License
//   version 2 along with this program; if not, write to the
//   Free Software Foundation, Inc.,
//   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#include "addblitterwidgets.h"

#include "blitterwidgets/directdrawblitter.h"
#include "blitterwidgets/direct3dblitter.h"

void addBlitterWidgets(auto_vector<BlitterWidget> &blitters, VideoBufferLocker vbl) {
	blitters.push_back(new Direct3DBlitter(vbl));
	blitters.push_back(new DirectDrawBlitter(vbl));
}
