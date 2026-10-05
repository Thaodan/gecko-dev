/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.mozilla.focus.menu

import android.view.View
import android.view.ViewGroup
import androidx.compose.ui.platform.ComposeView
import kotlinx.coroutines.CoroutineScope
import mozilla.components.compose.menu.store.MenuStore
import mozilla.components.concept.toolbar.Toolbar
import org.mozilla.focus.ui.theme.FocusTheme

/**
 * [Toolbar.Action] showing the browser menu button and when clicked the menu itself in a popup anchored to this button.
 */
class MenuToolbarAction(
    override val weight: () -> Int = { -1 },
    private val buildMenuStore: (scope: CoroutineScope, onDismiss: () -> Unit) -> MenuStore,
) : Toolbar.Action {

    override fun createView(parent: ViewGroup): View =
        ComposeView(parent.context).apply {
            setContent {
                FocusTheme {
                    MenuButton(buildMenuStore = buildMenuStore)
                }
            }
        }

    override fun bind(view: View) = Unit
}
