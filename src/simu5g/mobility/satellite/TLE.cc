//
// Copyright (C) 2021 Mario Franke <research@m-franke.net>
//
// Documentation for these modules is at http://sat.car2x.org/
//
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include "TLE.h"

namespace simu5g {

std::string TLE::get_satellite_name() const
{
    return this->satellite_name;
}

std::string TLE::get_tle_line1() const
{
    return this->tle_line1;
}

std::string TLE::get_tle_line2() const
{
    return this->tle_line2;
}

void TLE::set_satellite_name(const std::string& name)
{
    this->satellite_name = name;
}

void TLE::set_tle_line1(const std::string& line1)
{
    this->tle_line1 = line1;
}

void TLE::set_tle_line2(const std::string& line2)
{
    this->tle_line2 = line2;
}

} // namespace simu5g
