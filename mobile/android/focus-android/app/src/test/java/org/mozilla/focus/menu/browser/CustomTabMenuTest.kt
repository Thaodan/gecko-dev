/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.mozilla.focus.menu.browser

import mozilla.components.browser.state.state.BrowserState
import mozilla.components.browser.state.state.CustomTabConfig
import mozilla.components.browser.state.state.CustomTabMenuItem
import mozilla.components.browser.state.state.WebExtensionState
import mozilla.components.browser.state.state.createCustomTab
import mozilla.components.browser.state.state.createTab
import mozilla.components.browser.state.store.BrowserStore
import mozilla.components.compose.base.text.Text
import mozilla.components.compose.menu.data.MenuItem
import mozilla.components.compose.menu.ui.MenuItemState
import mozilla.components.concept.engine.webextension.Action
import mozilla.components.feature.webcompat.reporter.WebCompatReporterFeature.WEBCOMPAT_REPORTER_EXTENSION_ID
import mozilla.components.support.test.mock
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import org.mozilla.focus.R
import org.mozilla.focus.menu.CustomTabMenuItemTapped
import org.mozilla.focus.menu.MenuItemTapped
import org.mozilla.focus.menu.ToolbarMenu.CustomTabItem

private const val APP_NAME = "Focus"

class CustomTabMenuTest {
    private val customTab = createCustomTab("https://mozilla.org", id = "customTab")
    private val selectedTab = createTab("https://example.org", id = "tab", private = true)
    private val browserState =
        BrowserState(
            tabs = listOf(selectedTab),
            customTabs = listOf(customTab),
            selectedTabId = selectedTab.id,
        )

    @Test
    fun `WHEN not an onboarding tab THEN allow opening the page somewhere else`() {
        val items = menu(isOnboardingTab = false).currentMenuGroups().flatMap { it.items }

        assertTrue(items.hasItem(CustomTabItem.OpenInBrowser))
        assertTrue(items.hasItem(CustomTabItem.OpenInApp))
        assertEquals(
            Text.Resource(R.string.menu_open_with_default_browser2, listOf(APP_NAME)),
            items.item(CustomTabItem.OpenInBrowser).title,
        )
    }

    @Test
    fun `WHEN an onboarding tab THEN keep the page inside the custom tab`() {
        val items = menu(isOnboardingTab = true).currentMenuGroups().flatMap { it.items }

        assertFalse(items.hasItem(CustomTabItem.OpenInBrowser))
        assertFalse(items.hasItem(CustomTabItem.OpenInApp))
        assertTrue(items.hasItem(CustomTabItem.FindInPage))
        assertTrue(items.hasItem(CustomTabItem.AddToHomeScreen))
    }

    @Test
    fun `GIVEN a different selected tab WHEN building the menu THEN describe the custom tab`() {
        val state =
            browserState.copy(
                customTabs =
                    listOf(customTab.copy(content = customTab.content.copy(canGoBack = true, desktopMode = true))),
                tabs =
                    listOf(
                        selectedTab.copy(
                            content = selectedTab.content.copy(canGoBack = false, desktopMode = false, loading = true)
                        )
                    ),
            )
        val items = menu(state).currentMenuGroups().flatMap { it.items }

        assertEquals(MenuItemState.DEFAULT, items.item(CustomTabItem.Back).state)
        assertEquals(MenuItemState.ACTIVE, items.item(CustomTabItem.RequestDesktop(isChecked = false)).state)
        assertTrue(items.hasItem(CustomTabItem.Reload))
        assertFalse(items.hasItem(CustomTabItem.Stop))
    }

    @Test
    fun `GIVEN items provided by the caller WHEN building the menu THEN show them after the other ones`() {
        val callerItem = CustomTabMenuItem(name = "Open in caller", pendingIntent = mock())
        val state =
            browserState.copy(
                customTabs = listOf(customTab.copy(config = CustomTabConfig(menuItems = listOf(callerItem))))
            )
        val groups = menu(state).currentMenuGroups()
        val callerItems = groups.last().items

        assertTrue(groups.dropLast(1).flatMap { it.items }.none { it.onClickEvent is CustomTabMenuItemTapped })
        assertEquals(Text.String("Open in caller"), callerItems.single().title)
        assertEquals(CustomTabMenuItemTapped(callerItem), callerItems.single().onClickEvent)
    }

    @Test
    fun `GIVEN an enabled reporter WHEN building the menu THEN report the custom tab`() {
        val action = Action("Report broken site…", true, null, null, null, null) {}
        val extension =
            WebExtensionState(
                id = WEBCOMPAT_REPORTER_EXTENSION_ID,
                allowedInPrivateBrowsing = true,
                pageAction = action.copy(enabled = false),
            )
        val state =
            browserState.copy(
                extensions = mapOf(extension.id to extension),
                customTabs =
                    listOf(customTab.copy(extensionState = mapOf(extension.id to extension.copy(pageAction = action)))),
            )

        assertEquals(action, state.webCompatReporterAction(state.customTabs.single()))
        assertEquals(
            Text.String("Report broken site…"),
            menu(state).currentMenuGroups().flatMap { it.items }.item(CustomTabItem.ReportSiteIssue).title,
        )
    }

    private fun menu(
        state: BrowserState = browserState,
        isOnboardingTab: Boolean = false,
    ) =
        CustomTabMenu(
            browserStore = BrowserStore(state),
            customTabId = customTab.id,
            appName = APP_NAME,
            isOnboardingTab = isOnboardingTab,
            resources = mock(),
        )

    private fun List<MenuItem>.item(item: CustomTabItem) = single { it.onClickEvent == MenuItemTapped(item) }

    private fun List<MenuItem>.hasItem(item: CustomTabItem) = any { it.onClickEvent == MenuItemTapped(item) }
}
