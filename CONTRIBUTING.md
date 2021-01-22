# Pull Requests

* Make your commits as [atomic as possible](https://www.freshconsulting.com/atomic-commits/).
   * Fundamental question 1: what could we need to revert later?
   * Fundamental question 2: what could we need to cherry-pick?
   * Fundamental question 3: is there an _and_ in the commit message? -> split it!

* Adhere to the commit message [guidelines](https://chris.beams.io/posts/git-commit/):
   * Start with the module you are changing, ended with a ':'
      * Especially, if commiting to the [android](android) dir, take care to prefix commits with
	    "Android: ".
      * A good way to find common module descriptions is to look into git history of the project!
   * Keep the commit message short and in the form of "When applied, this commit will ' `<your commit message>`
     * Do _not_ end the subject line with a '.'.
     * Example: `warpdrive: increase fuel capacity to 100k`
