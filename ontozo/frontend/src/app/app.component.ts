import {Component} from '@angular/core';
// see https://fontawesome.com/icons/square-caret-down?s=solid
import {faSquareCaretDown, faSquareCaretUp, faToggleOff, faToggleOn} from '@fortawesome/free-solid-svg-icons';

@Component({
    selector: 'app-root',
    templateUrl: './app.component.html',
    styleUrls: ['./app.component.scss']
})
export class AppComponent {
    title = 'frontend';
    faSquareCaretDown = faSquareCaretDown;
    faSquareCaretUp = faSquareCaretUp;
    faToggleOff = faToggleOff
    faToggleOn = faToggleOn
}
