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

#pragma once

#include <ostream>
#include <string>

namespace simu5g {

class TLE {
public:
    TLE(std::string satellite_name, std::string tle_line1, std::string tle_line2)
        : satellite_name(satellite_name)
        , tle_line1(tle_line1)
        , tle_line2(tle_line2)
    {
    }

    TLE()
    {
    }

    std::string get_satellite_name() const;
    std::string get_tle_line1() const;
    std::string get_tle_line2() const;

    void set_satellite_name(const std::string& name);
    void set_tle_line1(const std::string& line1);
    void set_tle_line2(const std::string& line2);

    friend std::ostream& operator<< (std::ostream& os, const TLE& tle)
    {
        return os << tle.satellite_name << std::endl
                  << tle.tle_line1 << std::endl
                  << tle.tle_line2 << std::endl;
    }

    std::string satellite_name;
    std::string tle_line1;
    std::string tle_line2;
};

} // end namespace
