package com.hana.lintcheck

import com.android.tools.lint.client.api.IssueRegistry
import com.android.tools.lint.detector.api.CURRENT_API
import com.android.tools.lint.detector.api.Issue
import com.hana.lintcheck.detector.*

class SampleIssueRegistry : IssueRegistry() {

    override val issues: List<Issue>
        get() {
            return listOf(
                ComplianceDetector.ISSUE
            )
        }

    override val api: Int
        get() = CURRENT_API
}