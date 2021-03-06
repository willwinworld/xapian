/** @file dlhweight.cc
 * @brief Xapian::DLHWeight class - The DLH weighting scheme of the DFR framework.
 */
/* Copyright (C) 2013, 2014 Aarsh Shah
 * Copyright (C) 2016 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <config.h>

#include "xapian/weight.h"

#include "xapian/error.h"
#include "common/log2.h"
#include <algorithm>

using namespace std;

namespace Xapian {

DLHWeight *
DLHWeight::clone() const
{
    return new DLHWeight();
}

void
DLHWeight::init(double factor)
{
    double wdf_upper = get_wdf_upper_bound();
    if (wdf_upper == 0) {
	upper_bound = 0.0;
	return;
    }

    const double wdf_lower = 1.0;
    double len_upper = get_doclength_upper_bound();

    double min_wdf_to_len = wdf_lower / len_upper;

    double N = get_collection_size();
    double F = get_collection_freq();

    // Calculate constant values to be used in get_sumpart().
    log_constant = get_average_length() * N / F;
    wqf_product_factor = get_wqf() * factor;

    // Calculate values for the upper bound.
    /* An upper bound of the term used in the third log can be obtained by
       plugging in the upper bound of the length and differentiating the term
       w.r.t wdf which gives the value of wdf at which the function attains
       maximum value. */
    double wdf_var = min(wdf_upper, len_upper / 2.0);
    double max_product_1 = wdf_var * (1.0 - wdf_var / len_upper);
    /* An upper bound can also be obtained by taking the minimum and maximum
       wdf value in the formula as shown. */
    double max_product_2 = wdf_upper * (1.0 - min_wdf_to_len);
    /* Take the minimum of the two upper bounds. */
    double max_product = min(max_product_1, max_product_2);

    upper_bound =
	wdf_upper * log2(log_constant) / (wdf_upper + 0.5) +
	(len_upper - wdf_lower) * log2(1.0 - min_wdf_to_len) / (wdf_lower + 0.5) +
	0.5 * log2(2.0 * M_PI * max_product) / (wdf_lower + 0.5);
    if (rare(upper_bound < 0.0))
	upper_bound = 0.0;
    else
	upper_bound *= wqf_product_factor;
}

string
DLHWeight::name() const
{
    return "Xapian::DLHWeight";
}

string
DLHWeight::serialise() const
{
    return string();
}

DLHWeight *
DLHWeight::unserialise(const string& s) const
{
    if (rare(!s.empty()))
	throw Xapian::SerialisationError("Extra data in DLHWeight::unserialise()");
    return new DLHWeight();
}

double
DLHWeight::get_sumpart(Xapian::termcount wdf, Xapian::termcount len,
		       Xapian::termcount) const
{
    if (wdf == 0) return 0.0;

    double wdf_to_len = double(wdf) / len;
    double one_minus_wdf_to_len = 1.0 - wdf_to_len;

    double wt = wdf * log2(wdf_to_len * log_constant) +
		(len - wdf) * log2(one_minus_wdf_to_len) +
		0.5 * log2(2.0 * M_PI * wdf * one_minus_wdf_to_len);
    if (rare(wt <= 0.0)) return 0.0;

    return wqf_product_factor * wt / (wdf + 0.5);
}

double
DLHWeight::get_maxpart() const
{
    return upper_bound;
}

double
DLHWeight::get_sumextra(Xapian::termcount, Xapian::termcount) const
{
    return 0;
}

double
DLHWeight::get_maxextra() const
{
    return 0;
}

}
