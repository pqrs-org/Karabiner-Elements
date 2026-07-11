module.exports = async function closeUnansweredIssues({ github, context }) {
  // Issues created before this date have been unanswered for more than 30 days.
  const cutoff = new Date()
  cutoff.setUTCDate(cutoff.getUTCDate() - 30)

  const query = [
    `repo:${context.repo.owner}/${context.repo.repo}`,
    'is:issue',
    'is:open',
    'comments:0',
    `created:<${cutoff.toISOString().slice(0, 10)}`,
  ].join(' ')

  const { data } = await github.rest.search.issuesAndPullRequests({
    q: query,
    sort: 'created',
    order: 'asc',
    per_page: Math.min(Number(process.env.MAX_OPERATIONS), 100),
  })

  for (const issue of data.items) {
    await github.rest.issues.addLabels({
      ...context.repo,
      issue_number: issue.number,
      labels: ['stale'],
    })
    await github.rest.issues.update({
      ...context.repo,
      issue_number: issue.number,
      state: 'closed',
      state_reason: 'not_planned',
    })
  }
}
