<?php

class NodePropertyBlock extends Block {
  function __construct($plugin_name, array $data = array()) {
    parent::__construct($plugin_name, $data);

    $this->settings += array(
      'label' => 'hidden',
      'date_format' => 'short',
      'custom_date_format' => '',
      'timezone' => '',
    );
  }

  /**
   * {@inheritdoc}
   */
  function getTitle() {
    return !empty($this->settings['title']) ? check_plain($this->settings['title']) : t('Node property');
  }
  
  /**
   *  Builds the block's configuration form.
   */
  function form(&$form, &$form_state) {
    parent::form($form, $form_state);

    $settings = $this->settings;
    $form['property'] = array(
      '#title' => t('Property'),
      '#type' => 'select',
      '#options' => array(
        'nid' => t('Node ID'),
        'vid' => t('Revision ID'),
        'type' => t('Content type'),
        'title' => t('Title'),
        'uid' => t('Author ID'),
        'created' => t('Created'),
        'changed' => t('Changed'),
        'name' => t('Author username'),
      ),
      '#default_value' => $settings['property'],
      '#description' => t('The node property to be displayed.'),
      '#required' => TRUE,
    );

    $date_formats = array();
    $date_types = system_get_date_formats();
    foreach ($date_types as $machine_name => $date_type) {
      $date_formats[$machine_name] = $date_type['label'] . ': ' . format_date(REQUEST_TIME, $machine_name);
    }

    $form['date_format'] = array(
      '#type' => 'select',
      '#title' => t('Date format'),
      '#options' => $date_formats + array(
        'custom' => t('Custom'),
        'raw time ago' => t('Time ago'),
        'time ago' => t('Time ago (with "ago" appended)'),
        'raw time hence' => t('Time hence'),
        'time hence' => t('Time hence (with "hence" appended)'),
        'raw time span' => t('Time span (future dates have "-" prepended)'),
        'inverse time span' => t('Time span (past dates have "-" prepended)'),
        'time span' => t('Time span (with "ago/hence" appended)'),
      ),
      '#default_value' => isset($this->settings['date_format']) ? $this->settings['date_format'] : 'short',
      '#states' => array(
        'visible' => array(
          array(':input[name="property"]' => array('value' => 'created')),
          array(':input[name="property"]' => array('value' => 'changed')),
        ),
      ),
    );
    $form['custom_date_format'] = array(
      '#type' => 'textfield',
      '#title' => t('Custom date format'),
      '#description' => t('If "Custom", see the <a href="@url" target="_blank">PHP manual</a> for available options. Otherwise, enter the number of different time units to display, which defaults to 2.', array('@url' => 'https://www.php.net/manual/datetime.format.php#refsect1-datetime.format-parameters')),
      '#default_value' => isset($this->settings['custom_date_format']) ? $this->settings['custom_date_format'] : '',
    );
    $form['timezone'] = array(
      '#type' => 'select',
      '#title' => t('Timezone'),
      '#description' => t('Timezone to be used for date output.'),
      '#options' => array('' => t('- Default site/user timezone -')) + system_time_zones(FALSE),
      '#default_value' => $this->settings['timezone'],
      '#states' => array(
        'visible' => array(
          array(':input[name="property"]' => array('value' => 'created')),
          array(':input[name="property"]' => array('value' => 'changed')),
        ),
      ),
    );
    // Setup #states for all possible date_formats on the custom_date_format form element.
    foreach (array('custom', 'raw time ago', 'time ago', 'raw time hence', 'time hence', 'raw time span', 'time span', 'raw time span', 'inverse time span', 'time span') as $custom_date_possible) {
      $form['custom_date_format']['#states']['visible'][] = array(
        ':input[name="date_format"]' => array('value' => $custom_date_possible),
      );
    }

    return $form;
  }

  /**
   * Submit handler to save the form settings.
   */
  function formSubmit($form, &$form_state) {
    parent::formSubmit($form, $form_state);
    $values = $form_state['values'];
    $this->settings['property'] = $values['property'];
    $this->settings['date_format'] = $values['date_format'];
    $this->settings['custom_date_format'] = $values['custom_date_format'];
    $this->settings['timezone'] = $values['timezone'];
  }

  /**
   *  Sets block content on block view.
   */
  function getContent() {
    $settings = $this->settings;
    $node = $this->contexts['node']->data;
    switch ($this->settings['property']) {
      case 'created':
      case 'changed':
        return $this->formatDate($node->{$settings['property']});
      case 'title':
        return t($node->title);
      case 'name': 
        $uid = $node->uid;
        $user = user_load($uid);
        return $user->name;
      default:
        return $node->{$settings['property']};
    }
  }

  /**
   * Helper function to format date properties.
   * 
   * @param int $value
   *  The UNIX timestamp to be formatted. Borrowed from views handlers.
   */
  protected function formatDate($value) {
    $format = $this->settings['date_format'];
    if (in_array($format, array('custom', 'raw time ago', 'time ago', 'raw time hence', 'time hence', 'raw time span', 'time span', 'raw time span', 'inverse time span', 'time span'))) {
      $custom_format = $this->options['custom_date_format'];
    }

    if ($value) {
      $timezone = !empty($this->settings['timezone']) ? $this->settings['timezone'] : NULL;
      $time_diff = REQUEST_TIME - $value; // will be positive for a datetime in the past (ago), and negative for a datetime in the future (hence)
      switch ($format) {
        case 'raw time ago':
          return format_interval($time_diff, is_numeric($custom_format) ? $custom_format : 2);
        case 'time ago':
          return t('%time ago', array('%time' => format_interval($time_diff, is_numeric($custom_format) ? $custom_format : 2)));
        case 'raw time hence':
          return format_interval(-$time_diff, is_numeric($custom_format) ? $custom_format : 2);
        case 'time hence':
          return t('%time hence', array('%time' => format_interval(-$time_diff, is_numeric($custom_format) ? $custom_format : 2)));
        case 'raw time span':
          return ($time_diff < 0 ? '-' : '') . format_interval(abs($time_diff), is_numeric($custom_format) ? $custom_format : 2);
        case 'inverse time span':
          return ($time_diff > 0 ? '-' : '') . format_interval(abs($time_diff), is_numeric($custom_format) ? $custom_format : 2);
        case 'time span':
          return t(($time_diff < 0 ? '%time hence' : '%time ago'), array('%time' => format_interval(abs($time_diff), is_numeric($custom_format) ? $custom_format : 2)));
        case 'custom':
          if ($custom_format == 'r') {
            return format_date($value, $format, $custom_format, $timezone, 'en');
          }
          return format_date($value, $format, $custom_format, $timezone);
        default:
          return format_date($value, $format, '', $timezone);
      }
    }
  }
}