/* readonlysortedmap.vala
 *
 * Copyright (C) 2009-2011  Maciej Piechotka
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 *
 * Author:
 * 	Maciej Piechotka <uzytkownik2@gmail.com>
 */

public abstract class Gee.AbstractSortedMap<K, V> : AbstractMap<K,V>, SortedMap<K,V> {
	/**
	 * {@inheritDoc}
	 */
	public abstract SortedMap<K,V> head_map (K before);

	/**
	 * {@inheritDoc}
	 */
	public abstract SortedMap<K,V> tail_map (K after);

	/**
	 * {@inheritDoc}
	 */
	public abstract SortedMap<K,V> sub_map (K before, K after);

	/**
	 * {@inheritDoc}
	 */
	public abstract SortedSet<K> ascending_keys { owned get; }

	/**
	 * {@inheritDoc}
	 */
	public abstract SortedSet<Map.Entry<K,V>> ascending_entries { owned get; }

	private WeakRef _read_only_view;
	construct {
		_read_only_view = WeakRef(null);
	}

	/**
	 * The read-only view this map.
	 */
	public new SortedMap<K,V> read_only_view {
		owned get {
			SortedMap<K,V>? instance = (SortedMap<K,V>?)_read_only_view.get ();
			if (instance == null) {
				instance = new ReadOnlySortedMap<K,V> (this);
				_read_only_view.set (instance);
			}
			return instance;
		}
	}

	// Future-proofing
	internal new virtual void reserved0() {}
	internal new virtual void reserved1() {}
	internal new virtual void reserved2() {}
	internal new virtual void reserved3() {}
	internal new virtual void reserved4() {}
	internal new virtual void reserved5() {}
	internal new virtual void reserved6() {}
	internal new virtual void reserved7() {}
	internal new virtual void reserved8() {}
	internal new virtual void reserved9() {}
}

