/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.mozilla.focus.menu

import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.cancel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.test.runCurrent
import kotlinx.coroutines.test.runTest
import mozilla.components.compose.base.text.Text
import mozilla.components.compose.menu.data.MenuItemsGroup
import mozilla.components.compose.menu.data.StandardMenuItem
import mozilla.components.compose.menu.store.MenuAction
import mozilla.components.compose.menu.store.MenuStore
import mozilla.components.support.test.mock
import mozilla.components.support.test.whenever
import org.junit.Assert.assertEquals
import org.junit.Test
import org.mockito.Mockito.verify
import org.mockito.Mockito.verifyNoInteractions
import org.mozilla.focus.browser.integration.BrowserMenuController
import org.mozilla.focus.menu.browser.BrowserMenu

@OptIn(ExperimentalCoroutinesApi::class)
class MenuMiddlewareTest {
    private val menu: BrowserMenu = mock()
    private val controller: BrowserMenuController = mock()
    private val groups = MutableStateFlow<List<MenuItemsGroup>>(emptyList())
    private val settings =
        listOf(
            MenuItemsGroup.Row(
                id = "settings",
                items = listOf(StandardMenuItem(Text.String("Settings"), MenuItemTapped(ToolbarMenu.Item.Settings))),
            )
        )

    @Test
    fun `WHEN menu items change THEN update the store until the menu scope is cancelled`() = runTest {
        whenever(menu.menuGroups).thenReturn(groups)
        var dismissCount = 0
        val store =
            MenuStore(middleware = listOf(MenuMiddleware(menu, controller, { dismissCount++ }, backgroundScope)))
        runCurrent()

        groups.value = settings
        runCurrent()
        assertEquals(settings, store.state.menuGroups)

        backgroundScope.cancel()
        runCurrent()
        assertEquals(0, groups.subscriptionCount.value)
        groups.value = emptyList()
        runCurrent()
        assertEquals(settings, store.state.menuGroups)
        assertEquals(0, dismissCount)
        verifyNoInteractions(controller)
    }

    @Test
    fun `WHEN an item is tapped THEN delegate the action and dismiss once`() = runTest {
        whenever(menu.menuGroups).thenReturn(groups)
        var dismissCount = 0
        val store =
            MenuStore(middleware = listOf(MenuMiddleware(menu, controller, { dismissCount++ }, backgroundScope)))

        store.dispatch(MenuItemTapped(ToolbarMenu.Item.RequestDesktop(true)))

        verify(controller).handleMenuInteraction(ToolbarMenu.Item.RequestDesktop(true))
        assertEquals(1, dismissCount)
    }

    @Test
    fun `WHEN updating items THEN pass the action to the reducer without dismissing`() = runTest {
        whenever(menu.menuGroups).thenReturn(groups)
        var dismissCount = 0
        val store =
            MenuStore(middleware = listOf(MenuMiddleware(menu, controller, { dismissCount++ }, backgroundScope)))

        store.dispatch(MenuAction.Update(settings))

        assertEquals(settings, store.state.menuGroups)
        assertEquals(0, dismissCount)
        verifyNoInteractions(controller)
    }
}
