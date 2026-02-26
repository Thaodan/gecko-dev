/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.mozilla.conventions

import org.gradle.api.Plugin
import org.gradle.api.Project

class ProjectPlugin : Plugin<Project> {
    @Suppress("UNCHECKED_CAST")
    override fun apply(project: Project) {
        val mozconfig = project.gradle.extensions.extraProperties["mozconfig"] as Map<String, Any>
        val topobjdir = mozconfig["topobjdir"] as String
        val configureMavenRepositories = project.gradle.extensions.extraProperties["configureMavenRepositories"] as groovy.lang.Closure<*>

        project.repositories.apply {
            configureMavenRepositories.call(this)
            maven { setUrl("${topobjdir}/gradle/maven") }
        }
    }
}
