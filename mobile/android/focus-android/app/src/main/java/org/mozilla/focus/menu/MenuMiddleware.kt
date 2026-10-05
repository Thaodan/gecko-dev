/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.mozilla.focus.menu

import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.launch
import mozilla.components.compose.menu.store.MenuAction
import mozilla.components.compose.menu.store.MenuEvent
import mozilla.components.compose.menu.store.MenuState
import mozilla.components.compose.menu.store.MenuStore
import mozilla.components.lib.state.Middleware
import mozilla.components.lib.state.Store
import org.mozilla.focus.browser.integration.BrowserMenuController

/**
 * [MenuEvent] dispatched when the user taps a browser menu item.
 *
 * @property item The [ToolbarMenu.FocusMenuItem] the user tapped.
 */
data class MenuItemTapped(val item: ToolbarMenu.FocusMenuItem) : MenuEvent

/**
 * [MenuStore] middleware keeping the menu up to date and delegating all user interactions to [BrowserMenuController],
 * closing the menu afterwards.
 *
 * [scope] is expected to be cancelled once the menu is closed, which stops observing [menu].
 */
class MenuMiddleware(
    private val menu: MenuItems,
    private val controller: BrowserMenuController,
    private val onDismiss: () -> Unit,
    private val scope: CoroutineScope,
) : Middleware<MenuState, MenuAction> {

    override fun invoke(
        store: Store<MenuState, MenuAction>,
        next: (MenuAction) -> Unit,
        action: MenuAction,
    ) {
        when (action) {
            is MenuAction.Init -> observeMenuUpdates(store)

            is MenuItemTapped -> {
                controller.handleMenuInteraction(action.item)
                onDismiss()
            }

            else -> Unit
        }

        next(action)
    }

    private fun observeMenuUpdates(store: Store<MenuState, MenuAction>) = scope.launch {
        menu.menuGroups.collect { store.dispatch(MenuAction.Update(it)) }
    }
}
